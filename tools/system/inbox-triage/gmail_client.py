"""Read-only Gmail access for inbox triage.

Auth is ported from /home/randalk/src/formalizer/tools/system/daywiz/email-info.py
(token load -> refresh -> InstalledAppFlow fallback), reusing the same token and
credentials files so no new Google Cloud setup is required.

SAFETY: the OAuth scope is gmail.readonly, so Google rejects any mutation at the
API level. There are deliberately no modify / batchModify / trash / delete /
labels / send calls anywhere in this module. Fetching a message with
users.messages.get does NOT clear its UNREAD label.
"""

import os
import random
import time

from google.auth.transport.requests import Request
from google.oauth2.credentials import Credentials
from google_auth_oauthlib.flow import InstalledAppFlow
from googleapiclient.discovery import build
from googleapiclient.errors import HttpError

SCOPES = [
    "https://www.googleapis.com/auth/calendar.readonly",
    "https://www.googleapis.com/auth/gmail.readonly",
]

# Writing is a SEPARATE grant held in a SEPARATE file, deliberately.
#
# gmail.modify can mark read, archive, label and trash, but it cannot delete
# permanently -- that needs the far broader mail.google.com scope, which this
# tool never requests. Trashed mail therefore stays recoverable in Gmail.
#
# Keeping it out of the shared daywiz token means the read-only path is still
# enforced by Google rather than by this code: in read-only mode the write token
# is never loaded, so the process has no credential capable of mutation.
WRITE_SCOPES = ["https://www.googleapis.com/auth/gmail.modify"]

HOME = os.environ["HOME"]
TOKEN_PATH = os.path.join(HOME, ".formalizer/config/daywiz/token.json")
CREDENTIALS_PATH = os.path.join(HOME, ".formalizer/config/daywiz/credentials.json")
WRITE_TOKEN_PATH = os.path.join(HOME, ".formalizer/config/inbox-triage/gmail-write-token.json")

# Gmail's per-user ceiling is 250 quota units/sec, and different calls cost
# different amounts: messages.get and messages.list are 5 units, threads.get is
# 10. A batch must stay under the ceiling in UNITS, not in requests -- a batch of
# 25 threads is 250 units, lands exactly on the cap, and sheds ~1% of the batch
# as HTTP 403 userRateLimitExceeded.
BATCH_SIZE = 25          # 125 units for 5-unit calls
THREAD_BATCH_SIZE = 12   # 120 units for 10-unit threads.get
INTER_BATCH_PAUSE = 0.15

THREAD_METADATA_HEADERS = ["From", "To", "Date", "Subject", "Message-ID"]


class GmailError(RuntimeError):
    pass


def get_service():
    """Return an authorized, read-only Gmail service.

    Mirrors email-info.py:83-101 so both tools share one token file.
    """
    creds = None
    if os.path.exists(TOKEN_PATH):
        creds = Credentials.from_authorized_user_file(TOKEN_PATH, SCOPES)

    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            creds.refresh(Request())
        else:
            if not os.path.exists(CREDENTIALS_PATH):
                raise GmailError(
                    "No usable token and no credentials.json at %s" % CREDENTIALS_PATH
                )
            flow = InstalledAppFlow.from_client_secrets_file(CREDENTIALS_PATH, SCOPES)
            creds = flow.run_local_server(port=0)
        with open(TOKEN_PATH, "w") as fh:
            fh.write(creds.to_json())

    # cache_discovery=False avoids the noisy oauth2client file_cache warning.
    return build("gmail", "v1", credentials=creds, cache_discovery=False)


def write_token_exists():
    return os.path.exists(WRITE_TOKEN_PATH)


def authorize_write():
    """Run the consent flow for the write scope. Interactive: opens a browser."""
    if not os.path.exists(CREDENTIALS_PATH):
        raise GmailError("No credentials.json at %s" % CREDENTIALS_PATH)
    flow = InstalledAppFlow.from_client_secrets_file(CREDENTIALS_PATH, WRITE_SCOPES)
    creds = flow.run_local_server(
        port=0,
        authorization_prompt_message=(
            "Open this URL to authorize inbox-triage for gmail.modify:\n\n    {url}\n"
        ),
        success_message=(
            "inbox-triage is authorized to modify this mailbox. You can close this tab."
        ),
    )
    os.makedirs(os.path.dirname(WRITE_TOKEN_PATH), mode=0o700, exist_ok=True)
    with open(WRITE_TOKEN_PATH, "w") as fh:
        fh.write(creds.to_json())
    os.chmod(WRITE_TOKEN_PATH, 0o600)
    return WRITE_TOKEN_PATH


def get_write_service():
    """Authorized service that CAN modify the mailbox.

    Never called in read-only mode. Raises rather than prompting, so nothing
    can quietly acquire write access in the middle of a request.
    """
    if not write_token_exists():
        raise GmailError(
            "No write authorization yet. Run:  ./triage.py authorize-write"
        )
    creds = Credentials.from_authorized_user_file(WRITE_TOKEN_PATH, WRITE_SCOPES)
    if not creds.valid:
        if creds.expired and creds.refresh_token:
            creds.refresh(Request())
            with open(WRITE_TOKEN_PATH, "w") as fh:
                fh.write(creds.to_json())
        else:
            raise GmailError(
                "Write authorization expired. Run:  ./triage.py authorize-write"
            )
    granted = set(creds.scopes or [])
    if not granted.issubset(set(WRITE_SCOPES)):
        raise GmailError(
            "Write token carries unexpected scopes: %s" % ", ".join(sorted(granted))
        )
    return build("gmail", "v1", credentials=creds, cache_discovery=False)


def _retry(fn, attempts=5):
    """Run fn(), retrying on 429/5xx with exponential backoff."""
    delay = 1.0
    for attempt in range(attempts):
        try:
            return fn()
        except HttpError as err:
            status = getattr(err.resp, "status", None)
            if status not in (403, 429, 500, 502, 503, 504) or attempt == attempts - 1:
                raise
            time.sleep(delay + random.random())
            delay *= 2
    raise GmailError("retries exhausted")


def get_profile(service):
    return _retry(lambda: service.users().getProfile(userId="me").execute())


def list_query_ids(service, query, hard_cap=None):
    """Page through a Gmail search and return the matching message ids."""
    ids = []
    page_token = None
    while True:
        resp = _retry(
            lambda: service.users()
            .messages()
            .list(userId="me", q=query, maxResults=500, pageToken=page_token)
            .execute()
        )
        for msg in resp.get("messages", []):
            ids.append((msg["id"], msg.get("threadId")))
        page_token = resp.get("nextPageToken")
        if not page_token:
            break
        if hard_cap and len(ids) >= hard_cap:
            break
    return ids[:hard_cap] if hard_cap else ids


def _run_batch(service, build_request, keys, on_result, progress=None, label="",
               rounds=6, batch_size=BATCH_SIZE):
    """Execute build_request(key) for every key using batched HTTP requests.

    Sub-request failures inside a batch are reported to the callback rather than
    raised, so the outer _retry never sees them. Gmail sheds load with per-request
    rate-limit errors under bursty batches, which means those keys must be
    retried explicitly here or they are silently lost. Each round re-submits only
    the keys that failed, backing off between rounds.
    """
    pending = list(keys)
    total = len(pending)
    done = [0]
    failures = []

    for round_index in range(rounds):
        if not pending:
            break
        failures = []

        for start in range(0, len(pending), batch_size):
            chunk = pending[start : start + batch_size]

            def callback(request_id, response, exception, _chunk=chunk):
                key = _chunk[int(request_id)]
                if exception is not None:
                    failures.append((key, exception))
                else:
                    done[0] += 1
                    on_result(key, response)

            batch = service.new_batch_http_request(callback=callback)
            for idx, key in enumerate(chunk):
                batch.add(build_request(key), request_id=str(idx))
            _retry(batch.execute)

            if progress:
                progress(done[0], total, label)
            time.sleep(INTER_BATCH_PAUSE)

        pending = [key for key, _ in failures]
        if pending and round_index < rounds - 1:
            # Rate-limit rejections need real time to clear, not milliseconds.
            time.sleep(min(30.0, 2.0 * (2 ** round_index)))

    if progress:
        progress(done[0], total, label)
    return failures


def fetch_messages(service, message_ids, progress=None, fmt="full", headers=None):
    """Fetch messages for the given ids. Read-only.

    fmt="metadata" with a headers list is much cheaper when only envelope
    information is needed (used by the config seeder).
    """

    def build_request(mid):
        kwargs = {"userId": "me", "id": mid, "format": fmt}
        if fmt == "metadata" and headers:
            kwargs["metadataHeaders"] = headers
        return service.users().messages().get(**kwargs)

    out = {}
    errors = _run_batch(
        service,
        build_request,
        list(message_ids),
        lambda mid, resp: out.__setitem__(mid, resp),
        progress=progress,
        label="messages",
    )
    return out, errors


def fetch_threads(service, thread_ids, progress=None):
    """Fetch thread metadata (headers only, no bodies) for the given ids."""
    out = {}
    errors = _run_batch(
        service,
        lambda tid: service.users()
        .threads()
        .get(
            userId="me",
            id=tid,
            format="metadata",
            metadataHeaders=THREAD_METADATA_HEADERS,
        ),
        list(thread_ids),
        lambda tid, resp: out.__setitem__(tid, resp),
        progress=progress,
        label="threads",
        batch_size=THREAD_BATCH_SIZE,
    )
    return out, errors


def probe_correspondence(service, addresses, cap=25, progress=None):
    """For each address, count how much mail it sent you and how much you sent it.

    Counts are capped at `cap` -- we only need "stranger vs. regular", not exact
    totals, and capping keeps each query cheap. Two searches per address, both
    issued in batches.
    """
    from_counts = {}
    to_counts = {}
    addresses = list(addresses)

    keys = [("from", a) for a in addresses] + [("to", a) for a in addresses]

    def build_request(key):
        kind, addr = key
        query = "from:%s" % addr if kind == "from" else "from:me to:%s" % addr
        return (
            service.users()
            .messages()
            .list(userId="me", q=query, maxResults=cap, includeSpamTrash=False)
        )

    def on_result(key, resp):
        kind, addr = key
        n = len(resp.get("messages", []))
        (from_counts if kind == "from" else to_counts)[addr] = n

    errors = _run_batch(
        service, build_request, keys, on_result, progress=progress, label="senders"
    )

    return (
        {
            addr: {
                "from_count": from_counts.get(addr, 0),
                "i_wrote_to_count": to_counts.get(addr, 0),
                "capped_at": cap,
            }
            for addr in addresses
        },
        errors,
    )

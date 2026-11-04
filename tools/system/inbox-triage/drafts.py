"""Reply drafts: flagged in the app, written by Claude, created in Gmail by you.

The flow, one message at a time:

  flagged   You flag a message in Inbox Triage ("Draft reply") with a one-line
            intent. Stored here only; nothing reaches Gmail.
  written   In a Claude session you say "draft my flagged replies". Claude reads
            `triage.py draft-requests --json` and hands back the text with
            `triage.py draft-write <file.json>`. Still nothing reaches Gmail.
            You can read and edit the text in the app.
  created   You press "Create drafts" in the app (write mode armed). This is the
            only step that writes to Gmail, and it only creates drafts: nothing
            is sent. Each call is logged to cache/actions.log before it is made.
  sent      "Check drafts" (read-only) found the draft gone from Gmail and a
            reply of yours sent in the thread since the draft was made. The app
            then marks the original read in Gmail (write mode), because opening
            a draft only opens the compose window, never the message itself,
            so Gmail would otherwise keep it unread. It then leaves triage.
  gone      The draft is gone and nothing was sent: you deleted it. The message
            stays in triage; flag it again if you like.

Everything is kept in cache/draft_requests.json, keyed by Gmail message id, and
carries its own copy of the envelope, so a request survives the message dropping
out of triage after you reply.
"""

import base64
import datetime
import os
from email.message import EmailMessage

import store

DRAFTS_FILE = os.path.join(store.CACHE_DIR, "draft_requests.json")

STATUSES = ("flagged", "written", "created", "sent", "gone")

# The draft's message id goes in the compose parameter; Gmail opens that draft.
DRAFT_URL = "https://mail.google.com/mail/u/0/#drafts?compose=%s"


def _now():
    return datetime.datetime.now().isoformat(timespec="seconds")


def load():
    return store.load_json(DRAFTS_FILE, default={}) or {}


def save(data):
    store.save_json(DRAFTS_FILE, data)


def public(req):
    """The request as the browser and Claude see it."""
    out = dict(req)
    if req.get("draft_message_id"):
        out["draft_url"] = DRAFT_URL % req["draft_message_id"]
    return out


def counts(data=None):
    data = load() if data is None else data
    out = {s: 0 for s in STATUSES}
    for req in data.values():
        out[req.get("status", "flagged")] = out.get(req.get("status", "flagged"), 0) + 1
    return out


def flag(msg, intent):
    """Flag a cached message for drafting, or update the intent of one already flagged."""
    data = load()
    req = data.get(msg["id"])
    if req and req["status"] in ("created",):
        raise ValueError("A Gmail draft already exists for this message. "
                         "Change it in Gmail, or clear this request first.")
    headers = msg.get("headers") or {}
    base = {
        "id": msg["id"],
        "threadId": msg["threadId"],
        "subject": msg.get("subject") or "",
        "from_name": msg.get("from_name") or "",
        "from_addr": msg.get("from_addr") or "",
        "reply_to": [r["addr"] for r in msg.get("reply_to") or []],
        "to": [r["addr"] for r in msg.get("to") or []],
        "cc": [r["addr"] for r in msg.get("cc") or []],
        "date": msg.get("internalDate", 0),
        "message_id_header": (headers.get("message-id") or "").strip(),
        "references_header": (headers.get("references") or "").strip(),
    }
    if req and req["status"] in ("flagged", "written"):
        req.update(base)
        req["intent"] = intent
        req["updated_at"] = _now()
    else:  # new, or flagged again after a deleted draft
        req = dict(base, intent=intent, status="flagged", flagged_at=_now())
    data[msg["id"]] = req
    save(data)
    return req


def clear(msg_id):
    data = load()
    gone = data.pop(msg_id, None)
    save(data)
    return gone is not None


def set_text(msg_id, body, to=None, cc=None, subject=None, source="claude"):
    """Store the text of a reply. Allowed until the Gmail draft has been created."""
    data = load()
    req = data.get(msg_id)
    if not req:
        raise ValueError("no draft request for message %s" % msg_id)
    if req["status"] in ("created", "sent"):
        raise ValueError("the Gmail draft for %r was already created; edit it in Gmail"
                         % req["subject"][:60])
    body = (body or "").strip()
    if not body:
        raise ValueError("empty draft text for %r" % req["subject"][:60])
    req["body"] = body
    req["draft_to"] = list(to) if to else default_to(req)
    req["draft_cc"] = list(cc) if cc is not None else req.get("draft_cc", [])
    req["draft_subject"] = subject or req.get("draft_subject") or reply_subject(req["subject"])
    req["status"] = "written"
    req["written_at"] = _now()
    req["written_by"] = source
    save(data)
    return req


def default_to(req):
    return req["reply_to"][:1] or [req["from_addr"]]


def reply_subject(subject):
    subject = (subject or "").strip()
    return subject if subject.lower().startswith("re:") else ("Re: " + subject).strip()


def build_raw(req):
    """RFC 2822 reply, threaded on the original message."""
    em = EmailMessage()
    em["To"] = ", ".join(req.get("draft_to") or default_to(req))
    if req.get("draft_cc"):
        em["Cc"] = ", ".join(req["draft_cc"])
    em["Subject"] = req.get("draft_subject") or reply_subject(req["subject"])
    mid = req.get("message_id_header")
    if mid:
        em["In-Reply-To"] = mid
        em["References"] = (req.get("references_header", "") + " " + mid).strip()
    em.set_content(req["body"])
    return base64.urlsafe_b64encode(em.as_bytes()).decode("ascii")


def create(msg_ids, service, log):
    """Create Gmail drafts for written requests. Never sends anything.

    `service` is the write service, `log` the actions log writer. Returns
    (created, failed) lists of {id, subject, ...}.
    """
    data = load()
    created, failed = [], []
    for mid in msg_ids:
        req = data.get(mid)
        if not req or req["status"] != "written":
            failed.append({"id": mid, "error": "not a written draft request"})
            continue
        log({"action": "create_draft", "id": mid, "threadId": req["threadId"],
             "subject": req["subject"][:120], "to": req.get("draft_to")})
        try:
            resp = service.users().drafts().create(userId="me", body={
                "message": {"raw": build_raw(req), "threadId": req["threadId"]},
            }).execute()
        except Exception as exc:
            failed.append({"id": mid, "subject": req["subject"], "error": str(exc)[:300]})
            continue
        req["status"] = "created"
        req["draft_id"] = resp.get("id")
        req["draft_message_id"] = (resp.get("message") or {}).get("id")
        req["created_at"] = _now()
        req["created_ms"] = int(datetime.datetime.now().timestamp() * 1000)
        created.append(public(req))
    save(data)
    log({"action": "create_draft", "result": "done", "succeeded": len(created),
         "failed": len(failed)})
    return created, failed


def _plain_body(payload):
    """Plain text of a Gmail message payload (drafts are plain text when we make them)."""
    plain, htmls, attachments = [], [], []
    store._walk_parts(payload or {}, plain, htmls, attachments, {"has_calendar_part": False})
    body = "\n".join(p for p in plain if p).strip()
    if not body and htmls:
        body = store.strip_html("\n".join(htmls))
    return body


def _was_sent(service, req):
    """After the draft vanished: did a reply of yours go out in the thread?

    Sending a draft turns it into a SENT message in the same thread, so look for
    a SENT message there that is newer than the draft. Read-only.
    """
    try:
        thread = service.users().threads().get(
            userId="me", id=req["threadId"], format="minimal").execute()
    except Exception as exc:
        if "404" in str(exc):
            return False
        raise
    since = req.get("created_ms") or 0
    if not since and req.get("created_at"):
        since = int(datetime.datetime.fromisoformat(req["created_at"]).timestamp() * 1000)
    since -= 5 * 60 * 1000  # clock slack
    for m in thread.get("messages", []) or []:
        if "SENT" in (m.get("labelIds") or []) and int(m.get("internalDate", 0) or 0) >= since:
            return True
    return False


def check(service, only=None):
    """Read-only: refresh created drafts from Gmail.

    A draft that is gone was either sent (status "sent", message counts as
    replied) or deleted (status "gone"). Returns counts plus the ids of the
    newly sent ones, so the caller can mark them read in Gmail.
    """
    data = load()
    result = {"checked": 0, "sent": 0, "gone": 0, "changed": 0, "sent_ids": [],
              "still_draft": 0}
    for req in data.values():
        if req["status"] != "created" or not req.get("draft_id"):
            continue
        if only and req["id"] not in only:
            continue
        result["checked"] += 1
        try:
            resp = service.users().drafts().get(
                userId="me", id=req["draft_id"], format="full").execute()
        except Exception as exc:
            if "404" in str(exc):
                if _was_sent(service, req):
                    req["status"] = "sent"
                    req["sent_at"] = _now()
                    result["sent"] += 1
                    result["sent_ids"].append(req["id"])
                else:
                    req["status"] = "gone"
                    req["gone_at"] = _now()
                    result["gone"] += 1
                continue
            raise
        # A sent draft can still answer to its draft id: Gmail then returns the
        # sent message, labelled SENT and no longer DRAFT.
        labels = (resp.get("message") or {}).get("labelIds") or []
        if "SENT" in labels and "DRAFT" not in labels:
            req["status"] = "sent"
            req["sent_at"] = _now()
            result["sent"] += 1
            result["sent_ids"].append(req["id"])
            continue
        result["still_draft"] += 1
        message = resp.get("message") or {}
        req["draft_message_id"] = message.get("id") or req.get("draft_message_id")
        current = _plain_body(message.get("payload"))
        if current and current.strip() != (req.get("gmail_body") or req.get("body", "")).strip():
            req["gmail_body"] = current
            req["gmail_checked_at"] = _now()
            result["changed"] += 1
        req["checked_at"] = _now()
    save(data)
    return result

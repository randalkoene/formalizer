"""Normalization of raw Gmail payloads and the local JSON cache.

Everything written by this module lives under inbox-triage/cache/. Nothing here
touches the mailbox.
"""

import base64
import html
import json
import os
import re
from email.utils import getaddresses

HERE = os.path.dirname(os.path.realpath(__file__))
CACHE_DIR = os.path.join(HERE, "cache")

MESSAGES_FILE = os.path.join(CACHE_DIR, "messages.json")
THREADS_FILE = os.path.join(CACHE_DIR, "threads.json")
SENDERS_FILE = os.path.join(CACHE_DIR, "sender_history.json")
SCORES_FILE = os.path.join(CACHE_DIR, "scores.json")

# Headers that carry triage signal. Everything else is dropped to keep the
# cache small and readable.
KEEP_HEADERS = {
    "from",
    "to",
    "cc",
    "bcc",
    "reply-to",
    "sender",
    "subject",
    "date",
    "message-id",
    "in-reply-to",
    "references",
    "list-id",
    "list-unsubscribe",
    "list-post",
    "precedence",
    "auto-submitted",
    "authentication-results",
    "received-spf",
    "return-path",
    "delivered-to",
    "x-mailer",
    "x-autoreply",
    "x-failed-recipients",
}

BODY_LIMIT = 16000  # characters kept per message (scoring reads the first 8k)

_TAG_RE = re.compile(r"<[^>]+>")
_SCRIPT_RE = re.compile(r"(?is)<(script|style)\b.*?</\1>")
_WS_RE = re.compile(r"[ \t\r\f\v]+")
_BLANKS_RE = re.compile(r"\n{3,}")


def ensure_cache_dir():
    os.makedirs(CACHE_DIR, exist_ok=True)


def save_json(path, data):
    ensure_cache_dir()
    tmp = path + ".tmp"
    with open(tmp, "w") as fh:
        json.dump(data, fh, indent=1, ensure_ascii=False)
    os.replace(tmp, path)


def load_json(path, default=None):
    if not os.path.exists(path):
        return default
    with open(path) as fh:
        return json.load(fh)


def _decode(data):
    if not data:
        return ""
    return base64.urlsafe_b64decode(data.encode("ascii")).decode("utf-8", "replace")


def strip_html(raw):
    text = _SCRIPT_RE.sub(" ", raw)
    text = re.sub(r"(?i)<br\s*/?>", "\n", text)
    text = re.sub(r"(?i)</(p|div|tr|li|h[1-6])>", "\n", text)
    text = _TAG_RE.sub(" ", text)
    text = html.unescape(text)
    text = _WS_RE.sub(" ", text)
    return _BLANKS_RE.sub("\n\n", text).strip()


def _walk_parts(part, plain, htmls, attachments, flags):
    mime = (part.get("mimeType") or "").lower()
    filename = part.get("filename") or ""
    body = part.get("body") or {}

    if mime.startswith("text/calendar"):
        flags["has_calendar_part"] = True

    if filename:
        attachments.append(
            {
                "filename": filename,
                "mimeType": part.get("mimeType"),
                "size": body.get("size", 0),
            }
        )
    elif mime == "text/plain":
        plain.append(_decode(body.get("data")))
    elif mime == "text/html":
        htmls.append(_decode(body.get("data")))

    for sub in part.get("parts", []) or []:
        _walk_parts(sub, plain, htmls, attachments, flags)


def _addr_list(value):
    if not value:
        return []
    out = []
    for name, addr in getaddresses([value]):
        addr = (addr or "").strip().lower()
        if addr:
            out.append({"name": (name or "").strip(), "addr": addr})
    return out


def domain_of(addr):
    return addr.rsplit("@", 1)[-1].lower() if addr and "@" in addr else ""


def normalize(raw):
    """Turn a raw Gmail message resource into the flat dict the scorer expects."""
    payload = raw.get("payload") or {}

    headers = {}
    for header in payload.get("headers", []) or []:
        name = header.get("name", "").lower()
        if name in KEEP_HEADERS:
            # Keep the first occurrence; duplicates are rare and the first wins
            # in practice for the headers we care about.
            headers.setdefault(name, header.get("value", ""))

    plain, htmls, attachments = [], [], []
    flags = {"has_calendar_part": False}
    _walk_parts(payload, plain, htmls, attachments, flags)

    body = "\n".join(p for p in plain if p).strip()
    if not body and htmls:
        body = strip_html("\n".join(htmls))
    body = body[:BODY_LIMIT]

    from_list = _addr_list(headers.get("from", ""))
    from_entry = from_list[0] if from_list else {"name": "", "addr": ""}

    return {
        "id": raw.get("id"),
        "threadId": raw.get("threadId"),
        "labelIds": raw.get("labelIds", []) or [],
        "internalDate": int(raw.get("internalDate", 0) or 0),
        "snippet": html.unescape(raw.get("snippet", "") or ""),
        "subject": headers.get("subject", ""),
        "from_name": from_entry["name"],
        "from_addr": from_entry["addr"],
        "from_domain": domain_of(from_entry["addr"]),
        "to": _addr_list(headers.get("to", "")),
        "cc": _addr_list(headers.get("cc", "")),
        "reply_to": _addr_list(headers.get("reply-to", "")),
        "delivered_to": (headers.get("delivered-to", "") or "").strip().lower(),
        "body_text": body,
        "attachments": attachments,
        "has_calendar_part": flags["has_calendar_part"],
        "headers": headers,
    }


def normalize_thread(raw, me_addr):
    """Condense a thread resource into the context the scorer needs."""
    messages = raw.get("messages", []) or []
    me = me_addr.lower()

    my_message_ids = []
    my_epochs = []
    participants = set()
    epochs = []

    for msg in messages:
        hdrs = {
            h.get("name", "").lower(): h.get("value", "")
            for h in (msg.get("payload") or {}).get("headers", []) or []
        }
        sender = ""
        parsed = _addr_list(hdrs.get("from", ""))
        if parsed:
            sender = parsed[0]["addr"]
            participants.add(sender)

        epoch = int(msg.get("internalDate", 0) or 0)
        epochs.append(epoch)

        # SENT label is the reliable marker; the From header is the fallback for
        # messages sent from an alias.
        if "SENT" in (msg.get("labelIds") or []) or sender == me:
            my_epochs.append(epoch)
            mid = (hdrs.get("message-id") or "").strip()
            if mid:
                my_message_ids.append(mid)

    return {
        "message_count": len(messages),
        "i_sent_count": len(my_epochs),
        "my_message_ids": my_message_ids,
        "last_from_me": max(my_epochs) if my_epochs else 0,
        "first_epoch": min(epochs) if epochs else 0,
        "last_epoch": max(epochs) if epochs else 0,
        "participants": sorted(participants),
    }

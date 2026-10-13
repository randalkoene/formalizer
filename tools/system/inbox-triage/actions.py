"""Actions that modify Gmail.

Every one of these is reversible from Gmail itself:

  mark_read  removes UNREAD      -- undo by marking unread again
  archive    removes INBOX       -- undo from All Mail
  star       adds STARRED        -- undo by unstarring
  trash      moves to Trash      -- recoverable for ~30 days
  report_spam moves to Spam      -- recoverable from Spam

Nothing here can delete permanently: the write scope is gmail.modify, which
Google does not allow to bypass Trash.

Every call is written to cache/actions.log before the local cache is touched, so
there is always a record of what was done even if something later fails.
"""

import datetime
import json
import os
import threading

import gmail_client
import store

ACTIONS_LOG = os.path.join(store.CACHE_DIR, "actions.log")
_LOG_LOCK = threading.Lock()

# label changes applied by users.messages.batchModify
LABEL_ACTIONS = {
    "mark_read": {
        "label": "Mark read",
        "add": [], "remove": ["UNREAD"],
        "blurb": "Clears the unread flag. The message stays in your inbox.",
        "removes_from_triage": True,
        "destructive": False,
    },
    "archive": {
        "label": "Archive",
        "add": [], "remove": ["INBOX"],
        "blurb": "Removes it from the inbox. Still in All Mail and searchable.",
        "removes_from_triage": True,
        "destructive": False,
    },
    "star": {
        "label": "Star",
        "add": ["STARRED"], "remove": [],
        "blurb": "Flags it in Gmail. Stays unread and stays here.",
        "removes_from_triage": False,
        "destructive": False,
    },
    "report_spam": {
        "label": "Report spam",
        "add": ["SPAM"], "remove": ["INBOX"],
        "blurb": "Moves it to Spam, where Gmail keeps it for 30 days.",
        "removes_from_triage": True,
        "destructive": True,
    },
}

# users.messages.trash -- the documented call, rather than poking the TRASH label
TRASH_ACTION = {
    "label": "Trash",
    "blurb": "Moves it to Trash. Gmail keeps trashed mail for 30 days, so this is undoable.",
    "removes_from_triage": True,
    "destructive": True,
}

ALL_ACTIONS = dict(LABEL_ACTIONS, trash=TRASH_ACTION)

# What makes sense where. "all" deliberately has none: choosing an action should
# mean having looked at a category first.
PER_CATEGORY = {
    "needs_reply": ["star", "mark_read", "archive"],
    "grants": ["star", "mark_read", "archive"],
    "directed": ["star", "mark_read", "archive"],
    "threads": ["mark_read", "archive"],
    # Onboarding mail is kept on purpose -- resumes and applications get
    # referred back to -- so it is read and filed, never trashed.
    "onboarding": ["star", "mark_read", "archive"],
    "bulk": ["mark_read", "archive", "trash"],
    "automated": ["mark_read", "archive", "trash"],
    "spam": ["trash", "report_spam"],
}

# Selecting several at once is useful everywhere, including the categories where
# every message matters -- plenty of those only need a glance before being
# cleared. The one exception is the catch-all "All messages" view, which offers
# no actions at all, so there is nothing to select for.
#
# What differs per category is WHICH actions are offered: Needs Reply, Grants,
# Directed and Threads still have no trash, in batch exactly as singly.
MULTI_SELECT_CATEGORIES = set(PER_CATEGORY)


def describe(action):
    spec = ALL_ACTIONS.get(action)
    if not spec:
        raise ValueError("unknown action %r" % action)
    return {"key": action, "label": spec["label"], "blurb": spec["blurb"],
            "destructive": spec["destructive"]}


def available(category=None, tag=None):
    """Actions offered for a view. None for the catch-all 'all messages' view."""
    if category in PER_CATEGORY:
        keys = PER_CATEGORY[category]
    elif tag == "muted_list":
        keys = ["mark_read", "archive", "trash"]
    elif tag:
        keys = ["star", "mark_read", "archive"]
    else:
        return []
    return [describe(k) for k in keys]


def log(entry):
    store.ensure_cache_dir()
    entry = dict(entry, at=datetime.datetime.now().isoformat(timespec="seconds"))
    with _LOG_LOCK:
        with open(ACTIONS_LOG, "a") as fh:
            fh.write(json.dumps(entry) + "\n")
    return entry


def read_log(limit=200):
    if not os.path.exists(ACTIONS_LOG):
        return []
    out = []
    with open(ACTIONS_LOG) as fh:
        for line in fh:
            line = line.strip()
            if line:
                try:
                    out.append(json.loads(line))
                except ValueError:
                    pass
    return out[-limit:][::-1]


def apply(action, message_ids, dry_run=False, note=None):
    """Perform `action` on message_ids. Returns a result dict.

    Acquires the write credential only here, and only when actually doing it.
    """
    spec = ALL_ACTIONS.get(action)
    if not spec:
        raise ValueError("unknown action %r" % action)
    ids = [i for i in dict.fromkeys(message_ids) if i]
    if not ids:
        raise ValueError("no messages given")

    record = {"action": action, "count": len(ids), "ids": ids[:200],
              "dry_run": bool(dry_run), "note": note}
    if dry_run:
        return dict(record, ok=True, failed=[], logged=False)

    log(record)                      # written BEFORE the call, never after
    service = gmail_client.get_write_service()
    failed = []

    if action == "trash":
        errors = gmail_client._run_batch(
            service,
            lambda mid: service.users().messages().trash(userId="me", id=mid),
            ids,
            lambda mid, resp: None,
            label="trash",
        )
        failed = [str(k) for k, _ in errors]
    else:
        # batchModify takes up to 1000 ids and returns no body.
        for start in range(0, len(ids), 500):
            chunk = ids[start:start + 500]
            body = {"ids": chunk}
            if spec["add"]:
                body["addLabelIds"] = spec["add"]
            if spec["remove"]:
                body["removeLabelIds"] = spec["remove"]
            gmail_client._retry(
                lambda b=body: service.users().messages().batchModify(
                    userId="me", body=b).execute()
            )

    done = [i for i in ids if i not in set(failed)]
    log({"action": action, "result": "done", "succeeded": len(done),
         "failed": len(failed)})
    return {"ok": True, "action": action, "succeeded": done, "failed": failed,
            "removes_from_triage": spec["removes_from_triage"]}


def drop_from_cache(message_ids):
    """Forget messages that are no longer unread-in-inbox after an action."""
    gone = set(message_ids)
    messages = store.load_json(store.MESSAGES_FILE) or []
    kept = [m for m in messages if m["id"] not in gone]
    store.save_json(store.MESSAGES_FILE, kept)
    scores = store.load_json(store.SCORES_FILE) or {}
    for mid in gone:
        scores.pop(mid, None)
    store.save_json(store.SCORES_FILE, scores)
    return len(messages) - len(kept)

#!/usr/bin/env python3
"""Local web UI for inbox-triage. Diagnostic only.

Serves the same cached data the CLI reads, on 127.0.0.1.

Credentials, precisely:

  * Reads (sync, fetch, seed-config, single-message checks) run in a spawned
    triage.py subprocess. The server holds no credential for them at all, and
    the browser and the terminal execute exactly the same code path. They are
    bounded by the gmail.readonly scope.

  * Actions load the WRITE credential in this process, but only inside the
    action call itself and only once write mode has been armed. In read-only
    mode gmail_client.get_write_service() is never reached, so the process holds
    nothing capable of modifying the mailbox.

  * gmail_client is imported here only for a file-existence check and a path
    constant; importing it builds no client and reads no token.

  ./serve.py                 # http://127.0.0.1:8711
  ./serve.py --port 9000
  ./serve.py --no-browser
  ./serve.py --idle-exit 180 # quit after 3 idle hours

Normally started from the Formalizer dashboard (see inbox-triage-start) and
stopped with the Stop server button in the page.
"""

import argparse
import collections
import json
import mimetypes
import os
import posixpath
import re
import shlex
import signal
import subprocess
import sys
import threading
import time
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse

import actions as gmail_actions
import corrections
import gmail_client
import scoring
import store
import viewer
from scoring import (CATEGORY_LABELS, CATEGORY_ORDER, NEEDS_LABELS, NEEDS_ORDER,
                     TAG_LABELS, TAG_MUTED, TAG_ORDER)

# Write mode is OFF whenever this process starts, and disarms itself again after
# a short while. Arming is a deliberate act, not a setting you forget about.
ARM_MINUTES = 30


class WriteMode:
    def __init__(self, arm_minutes=ARM_MINUTES):
        self.lock = threading.Lock()
        self.until = 0.0
        self.arm_minutes = arm_minutes

    def armed(self):
        return time.time() < self.until

    def arm(self, on):
        with self.lock:
            self.until = (time.time() + self.arm_minutes * 60) if on else 0.0
        return self.status()

    def status(self):
        remaining = max(0, int(self.until - time.time()))
        return {
            "armed": remaining > 0,
            "remaining": remaining,
            "armMinutes": self.arm_minutes,
            "writeTokenPresent": gmail_client.write_token_exists(),
            "writeTokenPath": gmail_client.WRITE_TOKEN_PATH,
        }

    def require(self):
        if not gmail_client.write_token_exists():
            raise PermissionError(
                "No write authorization. Run:  ./triage.py authorize-write")
        if not self.armed():
            raise PermissionError(
                "Write mode is off on the server — it expired, or the server was "
                "restarted since you armed it. Click the badge to arm it again.")


WRITE = WriteMode()

HERE = os.path.dirname(os.path.realpath(__file__))
WEB_DIR = os.path.join(HERE, "web")
PID_FILE = os.path.join(store.CACHE_DIR, "serve.pid")

# Time of the most recent request, for --idle-exit.
LAST_REQUEST = time.time()


def _read_pid():
    try:
        with open(PID_FILE) as fh:
            pid = int(fh.read().strip())
    except (OSError, ValueError):
        return None
    try:
        os.kill(pid, 0)          # does it still exist?
    except OSError:
        return None
    return pid


def _port_holder(port):
    """PID listening on `port`, if any -- even one we did not start."""
    try:
        out = subprocess.run(["ss", "-lptnH", "sport = :%d" % port],
                             capture_output=True, text=True, timeout=5).stdout
    except Exception:
        return None
    found = re.search(r"pid=(\d+)", out)
    return int(found.group(1)) if found else None

class Data:
    """Cache holder. Reloads from disk when the files change underneath us."""

    def __init__(self):
        self.lock = threading.Lock()
        self.stamp = None
        self.messages = []
        self.scores = {}
        self.threads = {}
        self.senders = {}
        self.numbered = {}
        self.cat_entries = {}
        self.ordered = []

    def _disk_stamp(self):
        out = []
        for path in (store.MESSAGES_FILE, store.SCORES_FILE, store.THREADS_FILE):
            try:
                out.append(os.path.getmtime(path))
            except OSError:
                out.append(0)
        return tuple(out)

    def ensure(self):
        with self.lock:
            stamp = self._disk_stamp()
            if stamp == self.stamp and self.messages:
                return
            self.messages = store.load_json(store.MESSAGES_FILE) or []
            self.scores = store.load_json(store.SCORES_FILE) or {}
            self.threads = store.load_json(store.THREADS_FILE) or {}
            self.senders = store.load_json(store.SENDERS_FILE) or {}
            self.ordered, self.numbered, self.cat_entries = viewer.build_index(
                self.messages, self.scores
            )
            self.stamp = stamp

    def rescore(self):
        """Re-run scoring over the cache. Pure local computation, no network."""
        # Reload from disk FIRST. A fetch may have just replaced the cache, and
        # scoring the stale in-memory copy would leave every newly fetched
        # message unscored -- the indexer then drops them silently and the
        # category counts no longer add up to the message total.
        with self.lock:
            self.stamp = None
        self.ensure()
        with self.lock:
            cfg = scoring.load_config()
            scores = scoring.score_all(self.messages, self.threads, self.senders, cfg)
            store.save_json(store.SCORES_FILE, scores)
            self.stamp = None
        self.ensure()


DATA = Data()


def row(entry):
    msg, sc = entry["msg"], entry["score"]
    thread = DATA.threads.get(msg["threadId"]) or {}
    return {
        "n": entry["n"],
        "id": msg["id"],
        "category": entry["category"],
        "needs": sc.get("needs", "none"),
        "tags": sc.get("tags") or [],
        "score": sc["total"],
        "subject": msg["subject"] or "(no subject)",
        "fromName": msg["from_name"] or msg["from_addr"],
        "fromAddr": msg["from_addr"],
        "fromDomain": msg["from_domain"],
        "date": msg["internalDate"],
        "ageDays": sc["flags"].get("age_days", 0),
        "snippet": msg["snippet"][:200],
        "attachments": len(msg["attachments"]),
        "threadCount": thread.get("message_count", 1),
        "youSent": thread.get("i_sent_count", 0),
        "topReasons": [
            {"rule": r["rule"], "points": r["points"], "detail": r["detail"]}
            for r in sc["reasons"][:3]
        ],
        "flags": sc["flags"],
        # Lowercased haystack so client-side search stays cheap.
        "q": (
            msg["subject"] + " " + msg["from_name"] + " " + msg["from_addr"]
            + " " + msg["snippet"]
        ).lower(),
    }


def overview():
    total = len(DATA.messages)
    cats = []
    for cat in CATEGORY_ORDER:
        entries = DATA.cat_entries.get(cat, [])
        icon, label, blurb = CATEGORY_LABELS[cat]
        cats.append({
            "key": cat,
            "icon": icon,
            "label": label,
            "blurb": blurb,
            "count": len(entries),
            "share": (100.0 * len(entries) / total) if total else 0,
            "top": entries[0]["score"]["total"] if entries else None,
            "range": [entries[0]["n"], entries[-1]["n"]] if entries else None,
        })

    # Volume by month, for the activity chart.
    months = {}
    ages = []
    scores_hist = []
    sender_counts = {}
    domain_counts = {}
    rule_counts = {}
    oldest_actionable = []

    needs_counts = {k: 0 for k in NEEDS_ORDER}
    tag_counts = {k: 0 for k in TAG_ORDER + TAG_MUTED}
    list_candidates = {}
    alias_candidates = {}
    taught_lists = (scoring.load_config().get("_corrections") or {}).get("lists") or {}

    actionable = {"needs_reply", "grants", "directed"}
    for entry in DATA.ordered:
        msg, sc = entry["msg"], entry["score"]
        when = time.localtime(msg["internalDate"] / 1000.0)
        key = "%04d-%02d" % (when.tm_year, when.tm_mon)
        months[key] = months.get(key, 0) + 1
        ages.append(sc["flags"].get("age_days", 0))
        scores_hist.append(sc["total"])
        name = msg["from_name"] or msg["from_addr"]
        sender_counts[name] = sender_counts.get(name, 0) + 1
        if msg["from_domain"]:
            domain_counts[msg["from_domain"]] = domain_counts.get(msg["from_domain"], 0) + 1
        for r in sc["reasons"]:
            rule_counts[r["rule"]] = rule_counts.get(r["rule"], 0) + 1

        needs_counts[sc.get("needs", "none")] = needs_counts.get(sc.get("needs", "none"), 0) + 1
        for tag in sc.get("tags") or []:
            tag_counts[tag] = tag_counts.get(tag, 0) + 1

        # Lists sitting in the noise that you have not ruled on yet. A message
        # with no List-Id at all is not a list -- the recipient we see there is
        # usually another address of yours, which is a different question.
        if entry["category"] in ("bulk", "automated"):
            keys = sc["flags"].get("list_keys") or []
            if msg["headers"].get("list-id"):
                if not any(k in taught_lists for k in keys):
                    list_candidates[keys[0]] = list_candidates.get(keys[0], 0) + 1
            elif keys:
                alias_candidates[keys[0]] = alias_candidates.get(keys[0], 0) + 1
        if entry["category"] in actionable:
            oldest_actionable.append(entry)

    oldest_actionable.sort(key=lambda e: e["msg"]["internalDate"])

    def top(counter, k=12):
        return [{"name": n, "count": c}
                for n, c in sorted(counter.items(), key=lambda kv: -kv[1])[:k]]

    # Age buckets, in days.
    edges = [(0, 1, "today"), (1, 7, "1-7d"), (7, 30, "1-4w"),
             (30, 90, "1-3mo"), (90, 365, "3-12mo"), (365, 1e9, "1y+")]
    age_buckets = [
        {"label": lbl, "count": sum(1 for a in ages if lo <= a < hi)}
        for lo, hi, lbl in edges
    ]

    # Score histogram in fixed-width bins.
    lo, hi = -120, 180
    width = 20
    bins = []
    cursor = lo
    while cursor < hi:
        bins.append({
            "label": "%d" % cursor,
            "lo": cursor,
            "count": sum(1 for s in scores_hist if cursor <= s < cursor + width),
        })
        cursor += width

    actionable_count = sum(
        len(DATA.cat_entries.get(c, [])) for c in actionable
    )

    scored = sum(len(DATA.cat_entries.get(c, [])) for c in CATEGORY_ORDER)
    return {
        "total": total,
        "unscored": total - scored,
        "categories": cats,
        "needs": [
            {"key": k, "label": NEEDS_LABELS[k][0], "blurb": NEEDS_LABELS[k][1],
             "count": needs_counts.get(k, 0)}
            for k in NEEDS_ORDER
        ],
        "tags": [
            {"key": k, "label": TAG_LABELS[k], "count": tag_counts.get(k, 0)}
            for k in TAG_ORDER if tag_counts.get(k)
        ],
        "mutedTags": [
            {"key": k, "label": TAG_LABELS[k], "count": tag_counts.get(k, 0)}
            for k in TAG_MUTED if tag_counts.get(k)
        ],
        "listCandidates": [
            {"key": k, "count": v}
            for k, v in sorted(list_candidates.items(), key=lambda kv: -kv[1])[:14]
        ],
        "aliasCandidates": [
            {"key": k, "count": v}
            for k, v in sorted(alias_candidates.items(), key=lambda kv: -kv[1])[:8]
            if v >= 5
        ],
        "taughtLists": [
            {"key": k, "kind": (v or {}).get("kind"), "note": (v or {}).get("note")}
            for k, v in sorted(taught_lists.items())
        ],
        "staleDays": scoring.load_config()["thresholds"]["stale_days"],
        "actionsFor": {
            "categories": {c: gmail_actions.available(c) for c in CATEGORY_ORDER},
            "tags": {t: gmail_actions.available(None, t)
                     for t in TAG_ORDER + TAG_MUTED},
            "needs": {n: gmail_actions.available(None, "needs") for n in NEEDS_ORDER},
            "all": [],
        },
        "multiSelect": sorted(gmail_actions.MULTI_SELECT_CATEGORIES),
        "mode": WRITE.status(),
        "months": [{"label": k, "count": v} for k, v in sorted(months.items())][-18:],
        "ageBuckets": age_buckets,
        "scoreBins": bins,
        "topSenders": top(sender_counts),
        "topDomains": top(domain_counts),
        "topRules": top(rule_counts, 14),
        "actionable": actionable_count,
        "noise": total - actionable_count,
        "oldestActionable": [
            {"n": e["n"], "subject": e["msg"]["subject"] or "(no subject)",
             "from": e["msg"]["from_name"] or e["msg"]["from_addr"],
             "ageDays": e["score"]["flags"].get("age_days", 0),
             "category": e["category"]}
            for e in oldest_actionable[:8]
        ],
        "configPresent": os.path.exists(scoring.CONFIG_FILE),
        "cacheTime": max(DATA.stamp) if DATA.stamp else 0,
    }


def detail(n):
    entry = DATA.numbered.get(n)
    if not entry:
        return None
    msg, sc = entry["msg"], entry["score"]
    thread = DATA.threads.get(msg["threadId"]) or {}
    sender = DATA.senders.get(msg["from_addr"]) or {}

    cfg = scoring.load_config()
    corr = cfg.get("_corrections") or {}
    hit = corrections.lookup(corr, msg["from_addr"], msg["from_domain"], msg["id"])
    taught = None
    if hit:
        scope, kind, meta = hit
        taught = {"scope": scope, "kind": kind,
                  "category": meta.get("category"), "note": meta.get("note")}

    # Addresses this was sent to that you have not declared as yours. If one of
    # them is a forwarding alias, saying so fixes every message sent to it.
    me_set = cfg["_me_set"]
    not_me = [r["addr"] for r in (msg["to"] + msg["cc"]) if r["addr"] not in me_set]

    # The List-Unsubscribe URL, when it is a plain link rather than a mailto:.
    unsub = msg["headers"].get("list-unsubscribe") or ""
    unsub_url = None
    found = re.search(r"<(https?://[^>]+)>", unsub)
    if found:
        unsub_url = found.group(1)

    return {
        "n": entry["n"],
        "id": msg["id"],
        "threadId": msg["threadId"],
        "category": entry["category"],
        "score": sc["total"],
        "reasons": sc["reasons"],
        "flags": sc["flags"],
        "subject": msg["subject"] or "(no subject)",
        "fromName": msg["from_name"],
        "fromAddr": msg["from_addr"],
        "to": [r["addr"] for r in msg["to"]],
        "cc": [r["addr"] for r in msg["cc"]],
        "date": msg["internalDate"],
        "body": msg["body_text"] or msg["snippet"],
        "attachments": msg["attachments"],
        "labels": msg["labelIds"],
        "headers": msg["headers"],
        "thread": thread,
        "sender": sender,
        "actions": gmail_actions.available(entry["category"]),
        "taught": taught,
        "needs": sc.get("needs", "none"),
        "tags": sc.get("tags") or [],
        "listKeys": scoring.list_keys(msg, cfg),
        "taughtList": (lambda h: {"key": h[0], "kind": h[1], "note": (h[2] or {}).get("note")}
                       if h else None)(
            corrections.lookup_list(corr, scoring.list_keys(msg, cfg))),
        "notMeRecipients": not_me[:4],
        "kinds": corrections.KINDS,
        "listKinds": corrections.LIST_KINDS,
        "gmailUrl": "https://mail.google.com/mail/u/0/#all/%s" % msg["id"],
        "unsubscribeUrl": unsub_url,
    }


# Long-running CLI operations, surfaced in the browser with live progress.
# Spawned rather than imported so that the CLI remains the single code path and
# this process never holds Gmail credentials.
JOBS = {
    "sync": {
        "argv": ["sync"],
        "label": "Sync",
        "blurb": "Seconds. Drops anything you handled in Gmail and pulls only what is new. "
                 "This is the one you want after replying.",
        "touches_gmail": True,
    },
    "fetch": {
        "argv": ["fetch"],
        "label": "Full rebuild",
        "blurb": "Re-downloads every unread message from scratch. Minutes, not seconds — "
                 "use Sync unless the cache looks wrong.",
        "touches_gmail": True,
    },
    "fetch-force": {
        "argv": ["fetch", "--force"],
        "label": "Fetch, re-probing senders",
        "blurb": "As above, but re-probes correspondence history for every sender.",
        "touches_gmail": True,
    },
    "seed-config": {
        "argv": ["seed-config", "--force"],
        "label": "Rebuild config from history",
        "blurb": "Rescans Sent mail and OVERWRITES config.yaml. Your corrections.yaml is untouched.",
        "touches_gmail": True,
        "destructive": "This overwrites config.yaml, including any weights you have hand-tuned.",
    },
}

PROGRESS_RE = re.compile(r"(\w[\w ]*):\s*(\d+)/(\d+)")


class Job:
    """At most one CLI job at a time, with its output streamed to the browser."""

    def __init__(self):
        self.lock = threading.Lock()
        self.proc = None
        self.kind = None
        self.started = 0.0
        self.finished = 0.0
        self.returncode = None
        self.lines = collections.deque(maxlen=300)
        self.warnings = []
        self.progress = None
        self.cancelled = False
        self.stage = "idle"      # idle -> running -> scoring -> done
        self.summary = None

    def _proc_alive(self):
        return self.proc is not None and self.proc.poll() is None

    def is_running(self):
        """True until ALL post-processing is finished, not just the subprocess.

        Scoring runs after the child exits. Reporting 'not running' during that
        window made the UI stop polling early, so the log appeared to end at
        "Next: ./triage.py score" with the progress bar frozen short of 100%.
        """
        return self._proc_alive() or self.stage == "scoring"

    def start(self, kind):
        spec = JOBS.get(kind)
        if not spec:
            raise ValueError("unknown job %r" % kind)
        with self.lock:
            if self.is_running():
                raise RuntimeError("a job is already running")
            self.stage = "running"
            self.warnings = []
            self.summary = None
            self.kind = kind
            self.started = time.time()
            self.finished = 0.0
            self.returncode = None
            self.cancelled = False
            self.lines.clear()
            self.progress = None
            argv = [sys.executable, os.path.join(HERE, "triage.py")] + spec["argv"]
            self.lines.append("$ " + " ".join(shlex.quote(a) for a in argv[1:]))
            # Without this the child's stdout is block-buffered when piped, so
            # every normal line arrives at exit AFTER the unbuffered stderr
            # output -- the log reads out of order.
            env = dict(os.environ, PYTHONUNBUFFERED="1")
            self.proc = subprocess.Popen(
                argv, cwd=HERE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                bufsize=0, env=env,
            )
        threading.Thread(target=self._pump, daemon=True).start()
        return kind

    def _pump(self):
        proc = self.proc
        buf = b""
        try:
            while True:
                chunk = proc.stdout.read(1)
                if not chunk:
                    break
                # Progress is written with carriage returns, so split on both.
                if chunk in (b"\n", b"\r"):
                    text = buf.decode("utf-8", "replace").strip()
                    buf = b""
                    if text:
                        self._record(text)
                else:
                    buf += chunk
        except Exception as exc:  # pragma: no cover - defensive
            self._record("reader error: %s" % exc)
        finally:
            if buf:
                self._record(buf.decode("utf-8", "replace").strip())
            self.returncode = proc.wait()
            if self.returncode == 0 and not self.cancelled:
                self.stage = "scoring"
                self._record("Scoring the refreshed cache\u2026")
                try:
                    DATA.rescore()
                    self.summary = {
                        "messages": len(DATA.messages),
                        "threads": len(DATA.threads),
                        "senders": len(DATA.senders),
                        "actionable": sum(
                            len(DATA.cat_entries.get(c, []))
                            for c in ("needs_reply", "grants", "directed")
                        ),
                    }
                    self._record("Re-scored %d messages." % len(DATA.messages))
                except Exception as exc:
                    self._record("WARNING: re-score failed: %s" % exc)
            self.stage = "done"
            self.finished = time.time()

    def _record(self, text):
        text = (text or "").strip()
        if not text:
            return
        match = PROGRESS_RE.search(text)
        if match:
            self.progress = {
                "phase": match.group(1).strip(),
                "done": int(match.group(2)),
                "total": int(match.group(3)),
            }
            # Progress is written with a carriage return and no newline, so the
            # next real message gets concatenated onto it. Strip the progress
            # fragment and keep whatever else was on the line -- otherwise a
            # warning printed right after a progress tick is silently eaten.
            text = (text[: match.start()] + text[match.end():]).strip(" :\t")
            if not text:
                return
        self.lines.append(text)
        if "WARNING" in text or "Error" in text or "error" in text:
            self.warnings.append(text)

    def cancel(self):
        with self.lock:
            if self._proc_alive():
                self.cancelled = True
                self.proc.terminate()
                return True
        return False

    def status(self):
        running = self.is_running()
        return {
            "running": running,
            "stage": self.stage,
            "kind": self.kind,
            "label": (JOBS.get(self.kind) or {}).get("label"),
            "progress": self.progress,
            "lines": list(self.lines),
            "warnings": list(self.warnings),
            "summary": self.summary,
            "returncode": self.returncode,
            "cancelled": self.cancelled,
            "elapsed": round((time.time() if running else self.finished or time.time())
                             - self.started, 1) if self.started else 0,
            "jobs": [dict(key=k, **{kk: vv for kk, vv in v.items() if kk != "argv"})
                     for k, v in JOBS.items()],
        }


JOB = Job()


class Handler(BaseHTTPRequestHandler):
    server_version = "InboxTriage"

    def log_message(self, fmt, *args):
        # Quiet by default so the console stays readable; INBOX_TRIAGE_LOG=1
        # turns on an access log, which is how you catch a UI that is
        # re-fetching something on a timer.
        if os.environ.get("INBOX_TRIAGE_LOG"):
            sys.stderr.write("%s  %s\n" % (time.strftime("%H:%M:%S"), fmt % args))

    def _send(self, code, body, ctype="application/json; charset=utf-8"):
        if isinstance(body, (dict, list)):
            body = json.dumps(body).encode("utf-8")
        elif isinstance(body, str):
            body = body.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def _static(self, rel):
        # Confine to web/ -- no traversal out of the served directory.
        safe = posixpath.normpath("/" + rel).lstrip("/")
        path = os.path.join(WEB_DIR, safe)
        if not os.path.abspath(path).startswith(WEB_DIR) or not os.path.isfile(path):
            self._send(404, {"error": "not found"})
            return
        ctype = mimetypes.guess_type(path)[0] or "application/octet-stream"
        if ctype.startswith("text/") or ctype == "application/javascript":
            ctype += "; charset=utf-8"
        with open(path, "rb") as fh:
            self._send(200, fh.read(), ctype)

    def do_GET(self):
        global LAST_REQUEST
        LAST_REQUEST = time.time()
        url = urlparse(self.path)
        path = url.path

        # These must work before any cache exists -- otherwise a fresh install
        # could never run its first fetch from the browser.
        if path == "/":
            return self._static("index.html")
        if path == "/api/job":
            return self._send(200, JOB.status())
        if path == "/api/mode":
            return self._send(200, WRITE.status())
        if path == "/api/actions":
            return self._send(200, {"log": gmail_actions.read_log(limit=120)})
        if path == "/api/corrections":
            return self._send(200, {
                "corrections": corrections.load(),
                "kinds": corrections.KINDS,
        "listKinds": corrections.LIST_KINDS,
                "path": corrections.CORRECTIONS_FILE,
            })
        if not path.startswith("/api/"):
            return self._static(path.lstrip("/"))

        try:
            DATA.ensure()
        except Exception as exc:  # cache missing or malformed
            self._send(500, {"error": str(exc)})
            return
        if path == "/api/overview":
            ov = overview()
            ov["hasCache"] = bool(DATA.messages)
            ov["hasScores"] = bool(DATA.scores)
            return self._send(200, ov)
        if path == "/api/messages":
            return self._send(200, [row(e) for e in DATA.ordered])
        if path == "/api/weights":
            cfg = scoring.load_config()
            return self._send(200, {
                "weights": [
                    {"key": k, "default": v, "current": cfg["weights"].get(k, v)}
                    for k, v in scoring.DEFAULT_CONFIG["weights"].items()
                ],
                "thresholds": [
                    {"key": k, "default": v, "current": cfg["thresholds"].get(k, v)}
                    for k, v in scoring.DEFAULT_CONFIG["thresholds"].items()
                ],
                "configPath": scoring.CONFIG_FILE,
                "configPresent": os.path.exists(scoring.CONFIG_FILE),
            })
        match = re.fullmatch(r"/api/message/(\d+)", path)
        if match:
            found = detail(int(match.group(1)))
            if not found:
                return self._send(404, {"error": "no such message"})
            return self._send(200, found)
        if not path.startswith("/api/"):
            return self._static(path.lstrip("/"))
        self._send(404, {"error": "not found"})

    def _body(self):
        length = int(self.headers.get("Content-Length") or 0)
        if not length:
            return {}
        return json.loads(self.rfile.read(length) or b"{}")

    def _cross_origin(self):
        """True for a POST some other web page made from the browser.

        Without this, any page open in the same browser could post a simple
        text/plain request here -- arm write mode, act on mail, stop the server.
        Browsers always send Origin on a POST; curl and scripts do not, and are
        let through.
        """
        origin = self.headers.get("Origin")
        if origin is None:
            return False
        return urlparse(origin).netloc != (self.headers.get("Host") or "")

    def do_POST(self):
        global LAST_REQUEST
        LAST_REQUEST = time.time()
        path = urlparse(self.path).path
        if self._cross_origin():
            return self._send(403, {"error": "cross-origin request refused"})

        # Before DATA.ensure(): stopping must work even with a broken cache.
        if path == "/api/shutdown":
            if JOB.is_running():
                if not self._body().get("force"):
                    return self._send(409, {
                        "error": "A %s job is still running." % (JOB.status()["label"] or "CLI"),
                        "jobRunning": True,
                    })
                JOB.cancel()
            self._send(200, {"stopping": True})
            # shutdown() waits for serve_forever() to return, which cannot
            # happen inside this request's own thread.
            threading.Timer(0.2, self.server.shutdown).start()
            return

        try:
            DATA.ensure()
        except Exception as exc:
            return self._send(500, {"error": str(exc)})

        if path == "/api/job":
            try:
                JOB.start(self._body().get("kind"))
            except Exception as exc:
                return self._send(409, {"error": str(exc)})
            return self._send(200, JOB.status())

        if path == "/api/job/cancel":
            return self._send(200, {"cancelled": JOB.cancel(), **JOB.status()})

        if path == "/api/mode":
            try:
                body = self._body()
                if body.get("armed") and not gmail_client.write_token_exists():
                    raise PermissionError(
                        "No write authorization yet. Run:  ./triage.py authorize-write")
                return self._send(200, WRITE.arm(bool(body.get("armed"))))
            except PermissionError as exc:
                return self._send(403, {"error": str(exc)})
            except Exception as exc:
                return self._send(400, {"error": str(exc)})

        if path == "/api/act":
            try:
                body = self._body()
                action = body.get("action")
                ns = body.get("ns") or []
                entries = [DATA.numbered.get(int(n)) for n in ns]
                missing = [n for n, e in zip(ns, entries) if not e]
                if missing:
                    raise ValueError("unknown message number(s): %s" % missing[:5])
                if not entries:
                    raise ValueError("no messages selected")

                # Two separate questions, previously conflated:
                #
                #  1. Is acting allowed from this view at all? Only the
                #     catch-all "All messages" view is barred, by request.
                #  2. Does the action apply to THIS message? That is decided by
                #     the message's own category -- no trashing something out of
                #     Needs Reply -- not by whatever view you happened to use to
                #     find it. Judging by the view refused every action taken
                #     from a To reply / To read listing.
                if not (body.get("category") or body.get("tag") or body.get("needs")):
                    raise PermissionError(
                        "No actions from the All messages view — open a category first.")
                for e in entries:
                    applies = {a["key"] for a in gmail_actions.available(e["category"])}
                    if action not in applies:
                        raise PermissionError(
                            "%r does not apply to a %s message"
                            % (action, e["category"]))

                dry = bool(body.get("dryRun"))
                if not dry:
                    WRITE.require()
                ids = [e["msg"]["id"] for e in entries]
                result = gmail_actions.apply(action, ids, dry_run=dry, note="web")
                if not dry and result.get("removes_from_triage"):
                    gmail_actions.drop_from_cache(result["succeeded"])
                if not dry:
                    DATA.rescore()
                return self._send(200, {**result, "overview": overview()})
            except PermissionError as exc:
                return self._send(403, {"error": str(exc)})
            except Exception as exc:
                return self._send(400, {"error": str(exc)})

        # One-message reconcile, used right after replying in Gmail. Spawned
        # like the other reads, so this process holds no read credential.
        if path == "/api/sync-one":
            try:
                n = int(self._body().get("n"))
            except Exception:
                return self._send(400, {"error": "need a message number"})
            try:
                proc = subprocess.run(
                    [sys.executable, os.path.join(HERE, "triage.py"),
                     "sync-one", str(n), "--json"],
                    cwd=HERE, capture_output=True, text=True, timeout=45,
                    env=dict(os.environ, PYTHONUNBUFFERED="1"),
                )
                if proc.returncode != 0:
                    raise RuntimeError((proc.stderr or proc.stdout or "failed").strip()[:300])
                payload = json.loads(proc.stdout.strip().splitlines()[-1])
            except Exception as exc:
                return self._send(400, {"error": str(exc)})
            if payload.get("handled"):
                with DATA.lock:
                    DATA.stamp = None
                DATA.ensure()
                payload["overview"] = overview()
            return self._send(200, payload)

        if path == "/api/rescore":
            try:
                DATA.rescore()
            except Exception as exc:
                return self._send(500, {"error": str(exc)})
            return self._send(200, {"ok": True, "overview": overview()})

        # Teaching writes to corrections.yaml and re-scores the local cache.
        # Still no mailbox access of any kind.
        if path == "/api/teach":
            try:
                body = self._body()
                scope = body.get("scope", "sender")
                target = body.get("target")
                if not target:
                    raise ValueError("no target")
                if body.get("clear"):
                    corrections.teach(target, None, scope=scope)
                elif body.get("category"):
                    data = corrections.load()
                    data["messages"][str(target).lower()] = {
                        "category": body["category"],
                        "taught": __import__("datetime").date.today().isoformat(),
                    }
                    corrections.save(data)
                else:
                    corrections.teach(target, body.get("kind"), scope=scope,
                                      note=body.get("note"))
                DATA.rescore()
            except Exception as exc:
                return self._send(400, {"error": str(exc)})
            return self._send(200, {"ok": True, "overview": overview()})

        if path == "/api/alias":
            try:
                body = self._body()
                if not body.get("address"):
                    raise ValueError("no address")
                corrections.add_my_address(body["address"], remove=bool(body.get("remove")))
                DATA.rescore()
            except Exception as exc:
                return self._send(400, {"error": str(exc)})
            return self._send(200, {"ok": True, "overview": overview()})

        self._send(404, {"error": "not found"})


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", type=int, default=8711)
    parser.add_argument("--host", default="127.0.0.1",
                        help="default 127.0.0.1; this serves your mail, so keep it local")
    parser.add_argument("--no-browser", action="store_true")
    parser.add_argument("--arm-minutes", type=int, default=ARM_MINUTES,
                        help="how long write mode stays armed (default %d)" % ARM_MINUTES)
    parser.add_argument("--idle-exit", type=int, default=0, metavar="MINUTES",
                        help="exit after this many minutes without a request "
                             "(default 0: never)")
    parser.add_argument("--stop", action="store_true",
                        help="stop a running inbox-triage server and exit")
    parser.add_argument("--status", action="store_true",
                        help="say whether one is running, and exit")
    args = parser.parse_args()

    holder = _port_holder(args.port)

    if args.status:
        if holder:
            print("running — pid %d on port %d" % (holder, args.port))
            print("stop it with:  ./serve.py --stop")
        else:
            print("not running on port %d" % args.port)
        return

    if args.stop:
        if not holder:
            print("Nothing is listening on port %d." % args.port)
            return
        os.kill(holder, signal.SIGTERM)
        for _ in range(40):
            time.sleep(0.1)
            if not _port_holder(args.port):
                print("Stopped (pid %d)." % holder)
                return
        print("pid %d did not stop; try:  kill -9 %d" % (holder, holder))
        return

    if holder:
        # Far more useful than a bare "address already in use" traceback.
        try:
            cmd = open("/proc/%d/cmdline" % holder).read().replace("\0", " ").strip()
        except OSError:
            cmd = "?"
        raise SystemExit(
            "Port %d is already in use by pid %d:\n    %s\n\n"
            "That is probably an inbox-triage server still running — possibly a\n"
            "stale one, since Ctrl-C only stops a server running in the foreground.\n\n"
            "Stop it:   ./serve.py --stop\n"
            "Check it:  ./serve.py --status\n"
            "Or pick another port:  ./serve.py --port %d"
            % (args.port, holder, cmd, args.port + 1))

    WRITE.arm_minutes = max(1, args.arm_minutes)

    DATA.ensure()
    url = "http://%s:%d/" % (args.host, args.port)
    if not DATA.messages:
        print("inbox-triage  —  no cache yet; use Fetch new mail in the browser")
    else:
        print("inbox-triage  —  %d messages, read-only diagnostic view"
              % len(DATA.messages))
    print("   %s" % url)
    print("   Ctrl-C to stop")

    httpd = ThreadingHTTPServer((args.host, args.port), Handler)

    store.ensure_cache_dir()
    with open(PID_FILE, "w") as fh:
        fh.write(str(os.getpid()))

    def _shutdown(_sig, _frame):
        raise KeyboardInterrupt
    signal.signal(signal.SIGTERM, _shutdown)
    if not args.no_browser:
        def open_browser():
            # No browser (headless box, bare SSH) must not take down the server.
            try:
                webbrowser.open(url)
            except Exception:
                pass
        threading.Timer(0.5, open_browser).start()
    if args.idle_exit > 0:
        print("   exits after %d idle minutes" % args.idle_exit)

        def idle_watch():
            while True:
                time.sleep(30)
                if (not JOB.is_running()
                        and time.time() - LAST_REQUEST > args.idle_exit * 60):
                    print("idle for %d minutes" % args.idle_exit)
                    httpd.shutdown()
                    return
        threading.Thread(target=idle_watch, daemon=True).start()
    try:
        httpd.serve_forever()
        print("stopped")         # via Stop server, or --idle-exit
    except KeyboardInterrupt:
        print("\nstopped")
    finally:
        httpd.server_close()
        if _read_pid() == os.getpid():
            try:
                os.remove(PID_FILE)
            except OSError:
                pass


if __name__ == "__main__":
    main()

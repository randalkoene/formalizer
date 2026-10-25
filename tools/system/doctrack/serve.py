#!/usr/bin/env python3
"""Local web UI for doctrack, on 127.0.0.1:8713.

Serves the catalog in cache/doctrack.db: the review queue, the catalog with
search, work pages with their PDF, tags and the log of text handed out. Queue
decisions and tag merges are written to the same database the CLI uses.

Nothing here sends document text anywhere. Summarizing stays in a Claude Code
session ('doctrack.py batch'); the page only shows how many works wait for it.

  ./serve.py                 # http://127.0.0.1:8713, opens a browser
  ./serve.py --no-browser
  ./serve.py --idle-exit 180 # quit after 3 idle hours
  ./serve.py --status
  ./serve.py --stop

Normally started from the Formalizer dashboard (see doctrack-start.py) and
stopped with the power button in the page.
"""

import argparse
import json
import mimetypes
import os
import posixpath
import re
import signal
import sqlite3
import subprocess
import sys
import threading
import time
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse

import store

HERE = os.path.dirname(os.path.realpath(__file__))
WEB_DIR = os.path.join(HERE, "web")
PID_FILE = os.path.join(store.CACHE_DIR, "serve.pid")
DOCTRACK = os.path.join(HERE, "doctrack.py")
PORT = 8713

# Roots below which a work's PDF may be served, by source.
SOURCE_ROOTS = {"tex": os.path.realpath(os.path.expanduser("~/doc/tex"))}

# Time of the most recent request, for --idle-exit.
LAST_REQUEST = time.time()


def _read_pid():
    try:
        with open(PID_FILE) as fh:
            pid = int(fh.read().strip())
        os.kill(pid, 0)
        return pid
    except (OSError, ValueError):
        return None


def _port_holder(port):
    """PID listening on `port`, if any -- even one we did not start."""
    try:
        out = subprocess.run(["ss", "-lptnH", "sport = :%d" % port],
                             capture_output=True, text=True, timeout=5).stdout
    except Exception:
        return None
    found = re.search(r"pid=(\d+)", out)
    return int(found.group(1)) if found else None


# ---------- scan job ----------

class Job:
    """At most one 'doctrack.py scan' at a time, run as a subprocess."""

    def __init__(self):
        self.lock = threading.Lock()
        self.proc = None
        self.lines = []
        self.started = self.ended = None
        self.code = None

    def is_running(self):
        return self.proc is not None and self.proc.poll() is None

    def start(self):
        with self.lock:
            if self.is_running():
                return False
            self.lines, self.code = [], None
            self.started, self.ended = time.time(), None
            self.proc = subprocess.Popen([sys.executable, DOCTRACK, "scan"], stdout=subprocess.PIPE,
                                         stderr=subprocess.STDOUT, text=True, errors="replace")
            threading.Thread(target=self._pump, daemon=True).start()
            return True

    def _pump(self):
        for ln in self.proc.stdout:
            self.lines = (self.lines + [ln.rstrip()])[-200:]
        self.code = self.proc.wait()
        self.ended = time.time()

    def status(self):
        return {"running": self.is_running(), "started": self.started, "ended": self.ended,
                "code": self.code, "tail": self.lines[-12:]}


JOB = Job()


# ---------- data shaping ----------

def row(w, sizes, tags=None):
    return {
        "n": w["n"], "id": w["id"], "title": w["title"], "kind": w["kind"], "date": w["date"],
        "pages": w["pages"], "words": w["words"], "path": w["path"], "koene": bool(w["koene"]),
        "head": w["family"] == w["id"], "state": w["state"], "excluded": w["excluded"],
        "versions": sizes.get(w["family"], 1) - 1, "tags": tags if tags is not None else [],
    }


def all_tags(db):
    out = {}
    for r in db.execute("SELECT work_id, tag FROM work_tags ORDER BY tag"):
        out.setdefault(r["work_id"], []).append(r["tag"])
    return out


def overview(db):
    heads = store.works(db, "missing = 0 AND excluded = '' AND family = id")
    states = {s: 0 for s in store.STATES}
    kinds, decades = {}, {}
    for w in heads:
        states[w["state"]] = states.get(w["state"], 0) + 1
        kinds[w["kind"]] = kinds.get(w["kind"], 0) + 1
        d = w["date"][:3] + "0s"
        decades[d] = decades.get(d, 0) + 1
    last = db.execute("SELECT MAX(last_seen) FROM works").fetchone()[0]
    return {
        "families": len(heads),
        "works": db.execute("SELECT COUNT(*) FROM works WHERE missing = 0 AND excluded = ''").fetchone()[0],
        "excluded": db.execute("SELECT COUNT(*) FROM works WHERE missing = 0 AND excluded != ''").fetchone()[0],
        "states": states, "kinds": kinds, "decades": decades, "lastScan": last,
        "tags": db.execute("SELECT COUNT(*) FROM tags WHERE merged_into IS NULL").fetchone()[0],
        "sends": db.execute("SELECT COUNT(*) FROM sends").fetchone()[0],
    }


def list_works(db, q):
    one = lambda k: (q.get(k) or [""])[0]
    sizes, tags = store.family_sizes(db), all_tags(db)
    if one("q"):
        rows = store.search(db, one("q"), limit=200, mark=("\x01", "\x02"))
        out = []
        for w in rows:
            # The best hit may be an older version; show the family head with its snippet.
            r = row(store.work(db, w["family"]) or w, sizes, tags.get(w["family"], []))
            r["snippet"] = " ".join(w["snip"].split())
            out.append(r)
        return out
    where = ["missing = 0", "excluded != ''" if one("excluded") else "excluded = ''"]
    params = []
    if not one("versions"):
        where.append("family = id")
    for col in ("state", "kind"):
        if one(col):
            where.append(f"{col} = ?")
            params.append(one(col))
    if one("decade"):
        where.append("substr(date, 1, 3) = ?")
        params.append(one("decade")[:3])
    if one("tag"):
        where.append("family IN (SELECT work_id FROM work_tags WHERE tag = ?)")
        params.append(one("tag"))
    return [row(w, sizes, tags.get(w["family"], [])) for w in store.works(db, " AND ".join(where), params)]


def work_detail(db, n):
    w = store.work(db, n)
    if not w:
        return None
    sizes = store.family_sizes(db)
    out = row(w, sizes, store.tags_of(db, w["family"]))
    meta = json.loads(w["meta"] or "{}")
    out["meta"] = {k: meta.get(k, "") for k in ("docclass", "tex_author", "tex_date", "pdf_author",
                                                 "has_abstract")}
    out["dateSource"] = w["date_source"]
    out["files"] = [{"role": f["role"], "path": f["path"]}
                    for f in db.execute("SELECT role, path FROM files WHERE work_id = ?", (w["id"],))]
    out["hasPdf"] = any(f["role"] == "pdf" for f in out["files"])
    out["family"] = [row(v, sizes) for v in db.execute(
        "SELECT * FROM works WHERE family = ? AND missing = 0 ORDER BY date DESC", (w["family"],))]
    head = store.work(db, w["family"])
    out["headN"] = head["n"]
    out["headState"] = head["state"]
    s = store.summary_of(db, w["family"])
    out["summary"] = None if not s else {
        "title": s["title"], "keyPoints": json.loads(s["key_points"]), "dateEstimate": s["date_estimate"],
        "by": s["by"], "at": s["at"]}
    return out


def queue(db):
    sizes = store.family_sizes(db)
    folders = {}
    for w in store.queued(db):
        folders.setdefault(w["path"].split("/")[0], []).append(row(w, sizes))
    return {
        "folders": [{"folder": f, "works": folders[f]} for f in sorted(folders, key=str.lower)],
        "rules": [dict(r) for r in db.execute("SELECT * FROM rules ORDER BY prefix")],
        "approvedWaiting": len(store.works(db, "missing = 0 AND excluded = '' AND family = id "
                                                "AND state = 'approved'")),
    }


def tags(db):
    return [dict(t) for t in store.tag_counts(db)]


def sent(db):
    out = []
    for r in db.execute("SELECT * FROM sends ORDER BY id DESC"):
        ids = json.loads(r["work_ids"])
        works = [store.work(db, i) for i in ids]
        out.append({"at": r["at"], "purpose": r["purpose"], "chars": r["chars"],
                    "works": [{"n": w["n"], "title": w["title"]} if w else {"n": None, "title": i}
                              for i, w in zip(ids, works)]})
    return out


def pdf_path(db, n):
    """The PDF of work n, if it is one we catalogued and it lies under its source root."""
    w = store.work(db, n)
    if not w:
        return None
    f = db.execute("SELECT path FROM files WHERE work_id = ? AND role = 'pdf'", (w["id"],)).fetchone()
    root = SOURCE_ROOTS.get(w["source"])
    if not f or not root:
        return None
    real = os.path.realpath(f["path"])
    if not real.startswith(root + os.sep) or not real.lower().endswith(".pdf") or not os.path.isfile(real):
        return None
    return real


# ---------- HTTP ----------

class Handler(BaseHTTPRequestHandler):
    server_version = "doctrack"

    def log_message(self, fmt, *args):
        if os.environ.get("DOCTRACK_LOG"):
            sys.stderr.write("%s  %s\n" % (time.strftime("%H:%M:%S"), fmt % args))

    def _send(self, code, body, ctype="application/json; charset=utf-8", extra=None):
        if isinstance(body, (dict, list)):
            body = json.dumps(body).encode("utf-8")
        elif isinstance(body, str):
            body = body.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        for k, v in (extra or {}).items():
            self.send_header(k, v)
        self.end_headers()
        self.wfile.write(body)

    def _static(self, rel):
        # Confine to web/ -- no traversal out of the served directory.
        safe = posixpath.normpath("/" + rel).lstrip("/")
        path = os.path.join(WEB_DIR, safe)
        if not os.path.abspath(path).startswith(WEB_DIR + os.sep) or not os.path.isfile(path):
            return self._send(404, {"error": "not found"})
        ctype = mimetypes.guess_type(path)[0] or "application/octet-stream"
        if ctype.startswith("text/") or ctype == "application/javascript":
            ctype += "; charset=utf-8"
        with open(path, "rb") as fh:
            self._send(200, fh.read(), ctype)

    def do_GET(self):
        global LAST_REQUEST
        LAST_REQUEST = time.time()
        url = urlparse(self.path)
        path, q = url.path, parse_qs(url.query)
        if path == "/":
            return self._static("index.html")
        if path == "/api/job":
            return self._send(200, JOB.status())
        if not (path.startswith("/api/") or path.startswith("/pdf/")):
            return self._static(path.lstrip("/"))
        db = store.connect()
        try:
            if path == "/api/overview":
                return self._send(200, overview(db))
            if path == "/api/works":
                try:
                    return self._send(200, list_works(db, q))
                except sqlite3.OperationalError as e:
                    return self._send(400, {"error": f"search: {e}"})
            if path == "/api/queue":
                return self._send(200, queue(db))
            if path == "/api/tags":
                return self._send(200, tags(db))
            if path == "/api/sent":
                return self._send(200, sent(db))
            m = re.fullmatch(r"/api/work/(\d+)", path)
            if m:
                found = work_detail(db, int(m.group(1)))
                return self._send(200, found) if found else self._send(404, {"error": "no such work"})
            m = re.fullmatch(r"/pdf/(\d+)", path)
            if m:
                real = pdf_path(db, int(m.group(1)))
                if not real:
                    return self._send(404, {"error": "no PDF for this work"})
                with open(real, "rb") as fh:
                    name = os.path.basename(real).replace('"', "")
                    return self._send(200, fh.read(), "application/pdf",
                                      {"Content-Disposition": f'inline; filename="{name}"'})
            self._send(404, {"error": "not found"})
        finally:
            db.close()

    def _body(self):
        length = int(self.headers.get("Content-Length") or 0)
        return json.loads(self.rfile.read(length) or b"{}") if length else {}

    def _cross_origin(self):
        """True for a POST some other web page made from the browser.

        Browsers always send Origin on a POST; curl and scripts do not.
        """
        origin = self.headers.get("Origin")
        return origin is not None and urlparse(origin).netloc != (self.headers.get("Host") or "")

    def do_POST(self):
        global LAST_REQUEST
        LAST_REQUEST = time.time()
        path = urlparse(self.path).path
        if self._cross_origin():
            return self._send(403, {"error": "cross-origin request refused"})
        if path == "/api/shutdown":
            if JOB.is_running() and not self._body().get("force"):
                return self._send(409, {"error": "A scan is still running.", "jobRunning": True})
            self._send(200, {"ok": True})
            threading.Thread(target=self.server.shutdown, daemon=True).start()
            return
        if path == "/api/scan":
            started = JOB.start()
            return self._send(200 if started else 409,
                              {"ok": started, "error": None if started else "A scan is already running."})
        db = store.connect()
        try:
            body = self._body()
            if path == "/api/decide":
                state = body.get("state")
                if state not in ("approved", "ignored", "new"):
                    return self._send(400, {"error": "state must be approved, ignored or new"})
                try:
                    count = store.decide(db, state, [str(n) for n in body.get("works", [])],
                                         body.get("folder") or None, bool(body.get("allNew")))
                except ValueError as e:
                    return self._send(400, {"error": str(e)})
                db.commit()
                return self._send(200, {"ok": True, "count": count})
            if path == "/api/tag-merge":
                old, new = (body.get("old") or "").strip().lower(), (body.get("new") or "").strip().lower()
                if not old or not new or old == new:
                    return self._send(400, {"error": "give two different tags"})
                store.merge_tag(db, old, new)
                db.commit()
                return self._send(200, {"ok": True})
            self._send(404, {"error": "not found"})
        finally:
            db.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", type=int, default=PORT)
    parser.add_argument("--host", default="127.0.0.1",
                        help="default 127.0.0.1; this serves your documents, so keep it local")
    parser.add_argument("--no-browser", action="store_true")
    parser.add_argument("--idle-exit", type=int, default=0, metavar="MINUTES",
                        help="exit after this many minutes without a request (default 0: never)")
    parser.add_argument("--stop", action="store_true", help="stop a running doctrack server and exit")
    parser.add_argument("--status", action="store_true", help="say whether one is running, and exit")
    args = parser.parse_args()

    holder = _port_holder(args.port)
    if args.status:
        print(f"running, pid {holder} on port {args.port}" if holder else f"not running on port {args.port}")
        return
    if args.stop:
        if not holder:
            print(f"Nothing is listening on port {args.port}.")
            return
        os.kill(holder, signal.SIGTERM)
        for _ in range(40):
            time.sleep(0.1)
            if not _port_holder(args.port):
                print(f"Stopped (pid {holder}).")
                return
        print(f"pid {holder} did not stop; try:  kill -9 {holder}")
        return
    if holder:
        try:
            cmd = open(f"/proc/{holder}/cmdline").read().replace("\0", " ").strip()
        except OSError:
            cmd = "?"
        raise SystemExit(f"Port {args.port} is already in use by pid {holder}:\n    {cmd}\n\n"
                         "Probably a doctrack server still running.\n"
                         "Stop it:   ./serve.py --stop\nCheck it:  ./serve.py --status")

    db = store.connect()
    ov = overview(db)
    db.close()
    url = f"http://{args.host}:{args.port}/"
    print(f"doctrack  {ov['families']} works, {ov['states']['new']} waiting for review")
    print(f"   {url}")

    httpd = ThreadingHTTPServer((args.host, args.port), Handler)
    with open(PID_FILE, "w") as fh:
        fh.write(str(os.getpid()))

    def _shutdown(_sig, _frame):
        raise KeyboardInterrupt
    signal.signal(signal.SIGTERM, _shutdown)
    if not args.no_browser:
        def open_browser():
            try:
                webbrowser.open(url)
            except Exception:
                pass
        threading.Timer(0.5, open_browser).start()
    if args.idle_exit > 0:
        print(f"   exits after {args.idle_exit} idle minutes")

        def idle_watch():
            while True:
                time.sleep(30)
                if not JOB.is_running() and time.time() - LAST_REQUEST > args.idle_exit * 60:
                    print(f"idle for {args.idle_exit} minutes")
                    httpd.shutdown()
                    return
        threading.Thread(target=idle_watch, daemon=True).start()
    try:
        httpd.serve_forever()
        print("stopped")
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

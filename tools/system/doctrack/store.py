"""The doctrack catalog: one SQLite file in doctrack/cache/.

Tables
  works      one row per work (a paper, talk, abstract ...), any source
  files      the files a work consists of, with the signature that tells a
             re-scan whether anything changed
  fulltext   FTS5 index of title + extracted text, for keyword search
  rules      remembered queue decisions for whole folders
  summaries  what Claude wrote about a work: key points, tags, date estimate
  work_tags  tags per work (from summaries, editable)
  tags       the tag vocabulary; a tag may be merged into another
  sends      log of every work whose text was handed out for summarizing

Queue states (works.state), for family heads that are not excluded
  new         found, not reviewed yet
  approved    may be sent to Claude for summarizing
  ignored     stays in the catalog for search, never summarized
  summarized  has a summary for its current text
A summarized work whose text changes goes back to approved.
"""

import json
import os
import sqlite3
from datetime import datetime

HERE = os.path.dirname(os.path.realpath(__file__))
CACHE_DIR = os.path.join(HERE, "cache")
DB_FILE = os.environ.get("DOCTRACK_DB") or os.path.join(CACHE_DIR, "doctrack.db")

STATES = ("new", "approved", "ignored", "summarized")

SCHEMA = """
CREATE TABLE IF NOT EXISTS works (
    id          TEXT PRIMARY KEY,   -- source:path, e.g. tex:AGI-2010/koene.AGI-2010
    n           INTEGER UNIQUE,     -- short number for the command line, never reused
    source      TEXT NOT NULL,      -- tex (later: gdrive, slides, cv)
    path        TEXT NOT NULL,      -- below the source root
    title       TEXT,
    kind        TEXT,
    date        TEXT,               -- YYYY-MM-DD
    date_source TEXT,
    pages       INTEGER,
    words       INTEGER,
    koene       INTEGER,
    family      TEXT,               -- id of the family head (itself for a head)
    excluded    TEXT NOT NULL DEFAULT '',
    state       TEXT NOT NULL DEFAULT 'new',
    text_hash   TEXT,
    meta        TEXT,               -- JSON: source-specific facts
    first_seen  TEXT,
    last_seen   TEXT,
    missing     INTEGER NOT NULL DEFAULT 0
);
CREATE TABLE IF NOT EXISTS files (
    work_id TEXT NOT NULL,
    role    TEXT NOT NULL,          -- tex, pdf
    path    TEXT NOT NULL,
    mtime   REAL,
    size    INTEGER,
    PRIMARY KEY (work_id, role)
);
CREATE VIRTUAL TABLE IF NOT EXISTS fulltext USING fts5(work_id UNINDEXED, title, body);
CREATE TABLE IF NOT EXISTS rules (
    prefix   TEXT PRIMARY KEY,      -- work id prefix, e.g. tex:SFN-
    decision TEXT NOT NULL,         -- approved, ignored
    at       TEXT
);
CREATE TABLE IF NOT EXISTS summaries (
    work_id       TEXT PRIMARY KEY,
    title         TEXT,
    key_points    TEXT,             -- JSON list of strings
    date_estimate TEXT,
    by            TEXT,
    text_hash     TEXT,             -- of the text that was summarized
    at            TEXT
);
CREATE TABLE IF NOT EXISTS tags (
    name        TEXT PRIMARY KEY,
    description TEXT,
    merged_into TEXT
);
CREATE TABLE IF NOT EXISTS work_tags (
    work_id TEXT NOT NULL,
    tag     TEXT NOT NULL,
    PRIMARY KEY (work_id, tag)
);
CREATE TABLE IF NOT EXISTS sends (
    id       INTEGER PRIMARY KEY,
    at       TEXT,
    work_ids TEXT,                  -- JSON list
    chars    INTEGER,
    purpose  TEXT
);
"""

WORK_COLS = ("id", "source", "path", "title", "kind", "date", "date_source", "pages", "words",
             "koene", "family", "excluded", "text_hash", "meta")


def now():
    return datetime.now().isoformat(timespec="seconds")


def connect():
    os.makedirs(os.path.dirname(DB_FILE), exist_ok=True)
    db = sqlite3.connect(DB_FILE)
    db.row_factory = sqlite3.Row
    db.executescript(SCHEMA)
    return db


# ---------- works ----------

def works(db, where="1", params=()):
    return db.execute(f"SELECT * FROM works WHERE {where} ORDER BY date, id", params).fetchall()


def work(db, key):
    """A work by its number or id, or None."""
    if str(key).isdigit():
        return db.execute("SELECT * FROM works WHERE n = ?", (int(key),)).fetchone()
    return db.execute("SELECT * FROM works WHERE id = ?", (key,)).fetchone()


def file_sigs(db, source):
    """work id -> {role: (mtime, size)} for one source."""
    sigs = {}
    for r in db.execute("SELECT f.* FROM files f JOIN works w ON w.id = f.work_id WHERE w.source = ?",
                        (source,)):
        sigs.setdefault(r["work_id"], {})[r["role"]] = (r["mtime"], r["size"])
    return sigs


def save_work(db, w, files, text=None):
    """Insert or update a work. text=None keeps the stored text."""
    vals = {c: w.get(c) for c in WORK_COLS}
    vals["meta"] = json.dumps(w.get("meta", {}))
    prev = db.execute("SELECT n FROM works WHERE id = ?", (w["id"],)).fetchone()
    if prev:
        sets = ", ".join(f"{c} = :{c}" for c in WORK_COLS if c != "id")
        db.execute(f"UPDATE works SET {sets}, last_seen = :now, missing = 0 WHERE id = :id",
                   dict(vals, now=now()))
    else:
        n = db.execute("SELECT COALESCE(MAX(n), 0) + 1 FROM works").fetchone()[0]
        cols = ", ".join(WORK_COLS)
        db.execute(f"INSERT INTO works ({cols}, n, first_seen, last_seen) "
                   f"VALUES ({', '.join(':' + c for c in WORK_COLS)}, :n, :now, :now)",
                   dict(vals, n=n, now=now()))
    for role, (path, mtime, size) in files.items():
        db.execute("INSERT OR REPLACE INTO files VALUES (?, ?, ?, ?, ?)", (w["id"], role, path, mtime, size))
    if text is not None:
        db.execute("DELETE FROM fulltext WHERE work_id = ?", (w["id"],))
        db.execute("INSERT INTO fulltext VALUES (?, ?, ?)", (w["id"], w.get("title", ""), text))
    else:
        db.execute("UPDATE fulltext SET title = ? WHERE work_id = ?", (w.get("title", ""), w["id"]))


def text_of(db, work_id):
    r = db.execute("SELECT body FROM fulltext WHERE work_id = ?", (work_id,)).fetchone()
    return r["body"] if r else ""


def mark_missing(db, source, seen_ids):
    for r in db.execute("SELECT id FROM works WHERE source = ? AND missing = 0", (source,)).fetchall():
        if r["id"] not in seen_ids:
            db.execute("UPDATE works SET missing = 1 WHERE id = ?", (r["id"],))


# ---------- queue ----------

def rule_for(db, work_id):
    """The decision of the longest folder rule that covers work_id, or None."""
    r = db.execute("SELECT decision FROM rules WHERE ? LIKE prefix || '%' ORDER BY LENGTH(prefix) DESC LIMIT 1",
                   (work_id,)).fetchone()
    return r["decision"] if r else None


def refresh_states(db):
    """Bring queue states in line with families, rules and changed text."""
    for w in works(db, "excluded = '' AND missing = 0 AND family = id"):
        state = w["state"]
        if state == "new":
            state = rule_for(db, w["id"]) or "new"
        if state == "summarized":
            s = db.execute("SELECT text_hash FROM summaries WHERE work_id = ?", (w["id"],)).fetchone()
            if not s or s["text_hash"] != w["text_hash"]:
                state = "approved"
        if state != w["state"]:
            db.execute("UPDATE works SET state = ? WHERE id = ?", (state, w["id"]))


def set_state(db, ids, state):
    for wid in ids:
        db.execute("UPDATE works SET state = ? WHERE id = ?", (state, wid))


def add_rule(db, prefix, decision):
    db.execute("INSERT OR REPLACE INTO rules VALUES (?, ?, ?)", (prefix, decision, now()))


# ---------- summaries and tags ----------

def log_send(db, work_ids, chars, purpose):
    db.execute("INSERT INTO sends (at, work_ids, chars, purpose) VALUES (?, ?, ?, ?)",
               (now(), json.dumps(work_ids), chars, purpose))


def was_sent(db, work_id):
    return any(work_id in json.loads(r["work_ids"]) for r in db.execute("SELECT work_ids FROM sends"))


def canonical_tag(db, tag):
    tag = tag.strip().lower()
    seen = set()
    while tag not in seen:
        seen.add(tag)
        r = db.execute("SELECT merged_into FROM tags WHERE name = ?", (tag,)).fetchone()
        if not r or not r["merged_into"]:
            break
        tag = r["merged_into"]
    return tag


def save_summary(db, work_id, s, by):
    w = work(db, work_id)
    db.execute("INSERT OR REPLACE INTO summaries VALUES (?, ?, ?, ?, ?, ?, ?)",
               (work_id, s.get("title", ""), json.dumps(s.get("key_points", [])),
                s.get("date_estimate", ""), by, w["text_hash"], now()))
    db.execute("DELETE FROM work_tags WHERE work_id = ?", (work_id,))
    for t in s.get("tags", []):
        t = canonical_tag(db, t)
        db.execute("INSERT OR IGNORE INTO tags (name) VALUES (?)", (t,))
        db.execute("INSERT OR IGNORE INTO work_tags VALUES (?, ?)", (work_id, t))
    db.execute("UPDATE works SET state = 'summarized' WHERE id = ?", (work_id,))


def tag_counts(db):
    return db.execute("SELECT t.name, t.merged_into, COUNT(wt.work_id) AS n FROM tags t "
                      "LEFT JOIN work_tags wt ON wt.tag = t.name GROUP BY t.name ORDER BY n DESC, t.name"
                      ).fetchall()


def merge_tag(db, old, new):
    new = canonical_tag(db, new)
    db.execute("INSERT OR IGNORE INTO tags (name) VALUES (?)", (new,))
    db.execute("UPDATE tags SET merged_into = ? WHERE name = ?", (new, old))
    db.execute("UPDATE OR IGNORE work_tags SET tag = ? WHERE tag = ?", (new, old))
    db.execute("DELETE FROM work_tags WHERE tag = ?", (old,))


def tags_of(db, work_id):
    return [r["tag"] for r in db.execute("SELECT tag FROM work_tags WHERE work_id = ? ORDER BY tag", (work_id,))]


def summary_of(db, work_id):
    return db.execute("SELECT * FROM summaries WHERE work_id = ?", (work_id,)).fetchone()


# ---------- search ----------

def search(db, query, limit=20):
    return db.execute(
        "SELECT w.*, snippet(fulltext, 2, '[', ']', ' ... ', 12) AS snip FROM fulltext "
        "JOIN works w ON w.id = fulltext.work_id "
        "WHERE fulltext MATCH ? AND w.excluded = '' ORDER BY rank LIMIT ?", (query, limit)).fetchall()

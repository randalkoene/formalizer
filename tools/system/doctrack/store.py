"""The doctrack catalog: one SQLite file in doctrack/cache/.

Tables
  works      one row per work (a paper, talk, abstract ...), any source
  files      the files a work consists of, with the signature that tells a
             re-scan whether anything changed
  fulltext   FTS5 index of title + extracted text, for keyword search
  rules      remembered queue decisions for whole folders
  summaries  what Claude wrote about a work: key points, tags, date estimate
  work_tags  specific tags per work (from summaries, editable)
  work_areas broad areas per work, from the fixed list AREAS
  tags       the tag vocabulary; a tag may be merged into another
  sends      log of every work whose text was handed out for summarizing

Queue states (works.state), for family heads that are not excluded
  new         found, not reviewed yet
  approved    may be sent to Claude for summarizing
  ignored     stays in the catalog for search, never summarized
  aside       set apart from the catalog for later attention, e.g. CCF library
              items: not listed or searched with the catalog, never summarized
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

STATES = ("new", "approved", "ignored", "summarized", "aside")

# SQL condition for works whose family is not set aside (older versions keep
# state 'new'; the family head carries the decision).
NOT_ASIDE = "family NOT IN (SELECT id FROM works WHERE state = 'aside')"

# Broad areas: every summarized work gets one or two of these, beside its
# specific tags. Edit freely; a work keeps an area that is later removed here
# until it is summarized again.
AREAS = (
    "whole brain emulation",
    "neuroscience",
    "neurotechnology",
    "artificial intelligence",
    "mind and philosophy",
    "strategy and organization",
    "outreach and education",
)

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
    decision TEXT NOT NULL,         -- approved, ignored, aside
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
CREATE TABLE IF NOT EXISTS work_areas (
    work_id TEXT NOT NULL,
    area    TEXT NOT NULL,
    PRIMARY KEY (work_id, area)
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

# Work rows carry summary_title: the title in their family's summary, if any.
WORKS_WITH_SUMMARY_TITLE = ("SELECT works.*, s.title AS summary_title FROM works "
                            "LEFT JOIN summaries s ON s.work_id = works.family")


def works(db, where="1", params=()):
    return db.execute(f"{WORKS_WITH_SUMMARY_TITLE} WHERE {where} ORDER BY date, id", params).fetchall()


def work(db, key):
    """A work by its number or id, or None."""
    if str(key).isdigit():
        return db.execute(f"{WORKS_WITH_SUMMARY_TITLE} WHERE n = ?", (int(key),)).fetchone()
    return db.execute(f"{WORKS_WITH_SUMMARY_TITLE} WHERE works.id = ?", (key,)).fetchone()


def display_title(w):
    """The summary's cleaned-up title when there is one, else the title found by the scanner."""
    summary_title = w["summary_title"] if "summary_title" in w.keys() else None
    return summary_title or w["title"]


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

def location(w):
    """Where a work lives, as folder rules see it: 'tex:SFN-2003/...' or 'gdrive:My Drive/...'."""
    return f"{w['source']}:{w['path']}"


def queue_group(w):
    """The folder a work is listed under in the queue, as a rule prefix ending in '/'.

    TeX works group by their top folder; Drive works by drive and top folder.
    """
    parts = w["path"].split("/")[:-1]
    depth = 2 if w["source"] == "gdrive" else 1
    return f"{w['source']}:{'/'.join(parts[:depth])}/" if parts else f"{w['source']}:"


def group_label(prefix):
    return prefix.replace("gdrive:", "Drive: ", 1).replace("tex:", "", 1)


def rule_for(db, w):
    """The decision of the longest folder rule that covers work w, or None."""
    r = db.execute("SELECT decision FROM rules WHERE substr(?, 1, length(prefix)) = prefix "
                   "ORDER BY length(prefix) DESC LIMIT 1", (location(w),)).fetchone()
    return r["decision"] if r else None


def refresh_states(db):
    """Bring queue states in line with families, rules and changed text."""
    for w in works(db, "excluded = '' AND missing = 0 AND family = id"):
        state = w["state"]
        if state == "new":
            state = rule_for(db, w) or "new"
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


def queued(db):
    return works(db, "missing = 0 AND excluded = '' AND family = id AND state = 'new'")


def decide(db, state, keys=(), folder=None, all_new=False):
    """Set the queue state of works given by number or id, a folder prefix, or
    everything waiting. A folder decision is remembered as a rule; 'new' for a
    folder removes its rule. Returns the number of works changed; raises
    ValueError for a work that cannot be decided on. Does not commit."""
    if folder:
        prefix = folder if ":" in folder else "tex:" + folder
        if state == "new":
            db.execute("DELETE FROM rules WHERE prefix = ?", (prefix,))
        else:
            add_rule(db, prefix, state)
        hits = works(db, "missing = 0 AND excluded = '' AND family = id "
                     "AND substr(source || ':' || path, 1, length(?)) = ?", (prefix, prefix))
        targets = [w for w in hits if w["state"] in ("new", "approved", "ignored", "aside")]
    elif all_new:
        targets = queued(db)
    else:
        targets = []
        for k in keys:
            w = work(db, k)
            if not w:
                raise ValueError(f"no work {k}")
            if w["excluded"]:
                raise ValueError(f"{w['n']} is excluded ({w['excluded']})")
            if w["family"] != w["id"]:
                raise ValueError(f"{w['n']} is an older version; decide on its family head, "
                                 f"{work(db, w['family'])['n']}")
            targets.append(w)
    set_state(db, [w["id"] for w in targets], state)
    refresh_states(db)
    return len(targets)


def set_aside(db):
    return works(db, "missing = 0 AND excluded = '' AND family = id AND state = 'aside'")


def family_sizes(db):
    return {r["family"]: r["n"] for r in
            db.execute("SELECT family, COUNT(*) AS n FROM works WHERE missing = 0 GROUP BY family")}


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
        if t in AREAS:
            continue  # an area, not a specific tag
        db.execute("INSERT OR IGNORE INTO tags (name) VALUES (?)", (t,))
        db.execute("INSERT OR IGNORE INTO work_tags VALUES (?, ?)", (work_id, t))
    db.execute("DELETE FROM work_areas WHERE work_id = ?", (work_id,))
    for a in s.get("areas", []):
        db.execute("INSERT OR IGNORE INTO work_areas VALUES (?, ?)", (work_id, a.strip().lower()))
    db.execute("UPDATE works SET state = 'summarized' WHERE id = ?", (work_id,))


def tag_counts(db):
    return db.execute("SELECT t.name, t.merged_into, COUNT(wt.work_id) AS n FROM tags t "
                      "LEFT JOIN work_tags wt ON wt.tag = t.name GROUP BY t.name "
                      "HAVING n > 0 OR t.merged_into IS NOT NULL ORDER BY n DESC, t.name"
                      ).fetchall()


def merge_tag(db, old, new):
    new = canonical_tag(db, new)
    db.execute("INSERT OR IGNORE INTO tags (name) VALUES (?)", (new,))
    db.execute("UPDATE tags SET merged_into = ? WHERE name = ?", (new, old))
    db.execute("UPDATE OR IGNORE work_tags SET tag = ? WHERE tag = ?", (new, old))
    db.execute("DELETE FROM work_tags WHERE tag = ?", (old,))


def tags_of(db, work_id):
    return [r["tag"] for r in db.execute("SELECT tag FROM work_tags WHERE work_id = ? ORDER BY tag", (work_id,))]


def areas_of(db, work_id):
    return [r["area"] for r in db.execute("SELECT area FROM work_areas WHERE work_id = ? ORDER BY area",
                                          (work_id,))]


def area_counts(db):
    """Every area in AREAS with its number of works, plus any no longer listed."""
    counts = {a: 0 for a in AREAS}
    for r in db.execute("SELECT area, COUNT(*) AS n FROM work_areas GROUP BY area"):
        counts[r["area"]] = r["n"]
    return counts


def summary_of(db, work_id):
    return db.execute("SELECT * FROM summaries WHERE work_id = ?", (work_id,)).fetchone()


# ---------- search ----------

def search(db, query, limit=20, mark=("[", "]")):
    """Best match per family, best first. Raises sqlite3.OperationalError on a bad query."""
    rows, families = [], set()
    for w in db.execute(
            "SELECT w.*, snippet(fulltext, 2, ?, ?, ' ... ', 12) AS snip FROM fulltext "
            "JOIN works w ON w.id = fulltext.work_id "
            "WHERE fulltext MATCH ? AND w.excluded = '' AND w.missing = 0 AND w." + NOT_ASIDE +
            " ORDER BY rank LIMIT ?",
            (mark[0], mark[1], query, limit * 5)):
        if w["family"] not in families and len(rows) < limit:
            families.add(w["family"])
            rows.append(w)
    return rows

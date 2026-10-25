#!/usr/bin/env python3
"""doctrack: a catalog of Randal's own writings and presentations.

Everything it writes goes to doctrack/cache/doctrack.db. The only way document
text leaves this machine is the 'batch' command, which prints the text of
approved works for a Claude Code session to summarize, and logs what it printed.

Catalog
  doctrack.py scan                  find works in ~/doc/tex and Google Drive (incremental)
  doctrack.py scan --source tex     ... only one source (tex or drive)
  doctrack.py drive-auth            one-time read-only Google Drive authorization (opens a browser)
  doctrack.py list                  one line per work (family heads only)
  doctrack.py show <n|id>           everything known about one work
  doctrack.py search <words>        keyword search over the full text
  doctrack.py stats                 counts by kind, state, decade
  doctrack.py export <file>         TSV for review in a spreadsheet

Review queue
  doctrack.py queue                 works waiting for a decision, by folder
  doctrack.py approve <n>...        allow these to be summarized by Claude
  doctrack.py approve --folder P    ... every work whose path starts with P, now and later
                                    (P as 'queue' prints it, e.g. 'gdrive:My Drive/Papers/')
  doctrack.py ignore <n>...         keep for search, never summarize (also --folder)
  doctrack.py aside <n>...          set apart from the catalog for later attention, e.g.
                                    CCF library items; never summarized (also --folder;
                                    see them with: list --state aside)
  doctrack.py unreview <n>...       back to the queue
  doctrack.py rules                 the remembered folder decisions

Summaries (in a Claude Code session)
  doctrack.py batch [--n 3]         print approved, unsummarized works for summarizing
  doctrack.py summarize <file.json> store the summaries written for a batch
  doctrack.py sent                  what text has been handed out, and when
  doctrack.py tags                  the tag vocabulary with counts
  doctrack.py tag-merge <old> <new> fold one tag into another

Design: Node 20261015131453.1 and its linked Claude Doc.
"""

import argparse
import hashlib
import json
import os
import sys

import scanner
import store

DEFAULT_ROOT = os.path.expanduser("~/doc/tex")
STATE_MARK = {"new": "N", "approved": "A", "ignored": "I", "summarized": "S", "aside": "X"}


# ---------- scan ----------

def scan(db, root, sources=("tex", "drive")):
    """Bring the store up to date from each source, then regroup families."""
    if "tex" in sources:
        scan_tex(db, root)
    if "drive" in sources:
        import drive
        if drive.have_token():
            from googleapiclient.errors import HttpError
            try:
                scan_drive(db)
            except HttpError as e:
                db.commit()  # keep what was read before the error
                print(f"Google Drive: scan stopped, HTTP {e.resp.status}: {e.reason}")
        else:
            print("Google Drive: no token yet, skipped (run: doctrack.py drive-auth)")
    live = [dict(r) for r in store.works(db, "missing = 0")]
    texts = {w["id"]: store.text_of(db, w["id"]) for w in live}
    for wid, head in scanner.families(live, texts).items():
        db.execute("UPDATE works SET family = ? WHERE id = ?", (head, wid))
    store.refresh_states(db)
    db.commit()
    heads = db.execute("SELECT COUNT(*) FROM works WHERE missing = 0 AND excluded = '' "
                       "AND family = id").fetchone()[0]
    print(f"{heads} family heads in the catalog.")


def scan_tex(db, root):
    """Find works below root and bring their store entries up to date."""
    sigs = store.file_sigs(db, "tex")
    seen, read, kept = set(), 0, 0
    for tex, pdf in scanner.find_mains(root):
        path = os.path.relpath(tex, root)[:-4]
        wid = "tex:" + path
        seen.add(wid)
        ts, ps = os.stat(tex), os.stat(pdf)
        files = {"tex": (tex, ts.st_mtime, ts.st_size), "pdf": (pdf, ps.st_mtime, ps.st_size)}
        old_sig = sigs.get(wid, {})
        prev = store.work(db, wid)
        meta = json.loads(prev["meta"]) if prev else {}
        unchanged = old_sig.get("tex") == (ts.st_mtime, ts.st_size) and \
            old_sig.get("pdf") == (ps.st_mtime, ps.st_size) and \
            meta.get("text_version") == scanner.TEXT_VERSION
        w = {"id": wid, "source": "tex", "path": path, "text_version": scanner.TEXT_VERSION}
        w.update(scanner.parse_tex(tex))  # cheap, and picks up parser improvements
        if unchanged:
            w.update({k: meta[k] for k in ("pdf_title", "pdf_author", "pdf_created", "pdf_mtime", "first_line")})
            w.update({k: prev[k] for k in ("pages", "words", "koene")})
            w["text_hash"] = prev["text_hash"]
            text = None
            kept += 1
            scanner.classify(w)
        else:
            print(f"  reading {path}", file=sys.stderr)
            facts, text = scanner.read_pdf(pdf, w["tex_author"])
            w.update(facts)
            scanner.classify(w)
            if w["kind"] == "slides":
                text = scanner.clean_slides(text)
                w["words"] = len(text.split())
            w["text_hash"] = hashlib.sha1(text.encode()).hexdigest()
            read += 1
        w["meta"] = {k: v for k, v in w.items() if k not in store.WORK_COLS}
        w["koene"] = int(w["koene"])
        w["family"] = prev["family"] if prev else wid
        store.save_work(db, w, files, text)
    store.mark_missing(db, "tex", seen)
    db.commit()
    gone = len(sigs.keys() - seen)
    print(f"~/doc/tex: {len(seen)} works ({read} read, {kept} unchanged{f', {gone} gone' if gone else ''}).")


def scan_drive(db):
    """Find works in Google Drive and bring their store entries up to date.

    Titles of files being read go to stderr only, so that a summary on stdout
    shows counts and no document names.
    """
    import drive
    svc = drive.service()
    print("Google Drive: listing folders and files ...", file=sys.stderr)
    folder_path = drive.folder_paths(svc)
    files = drive.list_files(svc)
    sigs = store.file_sigs(db, "gdrive")
    seen, counts = set(), {"read": 0, "unchanged": 0, "not mine": 0, "excluded": 0}
    for i, f in enumerate(files, 1):
        folder = folder_path((f.get("parents") or [None])[0]) if f.get("parents") else "Shared with me"
        wid = "gdrive:" + f["id"]
        mtime = drive.epoch(f["modifiedTime"])
        prev = store.work(db, wid)
        meta = json.loads(prev["meta"]) if prev else {}
        unchanged = sigs.get(wid, {}).get("gdrive", (None,))[0] == mtime and \
            meta.get("text_version") == scanner.TEXT_VERSION
        if unchanged and meta.get("mine") is False:
            counts["not mine"] += 1
            seen.add(wid)
            continue
        mine = meta.get("mine") if unchanged else None
        if mine is None:
            mine = bool(f.get("ownedByMe") or f.get("lastModifyingUser", {}).get("me")
                        or any(o.get("me") for o in f.get("owners", [])))
        name_reason = drive.exclusion_by_name(f["name"], folder)
        if not mine and not name_reason:
            mine = drive.edited_by_me(svc, f["id"])
        w = {"id": wid, "source": "gdrive", "path": f"{folder}/{f['name']}", "title": f["name"],
             "kind": drive.kind_of(f, folder), "date": f["createdTime"][:10], "date_source": "drive-created",
             "pages": 0, "koene": 1, "text_version": scanner.TEXT_VERSION, "mine": mine,
             "mime": f["mimeType"], "web_url": f.get("webViewLink", ""), "modified": f["modifiedTime"],
             "owners": [o.get("displayName", "") for o in f.get("owners", [])]}
        sig = {"gdrive": (f["id"], mtime, 0)}
        seen.add(wid)
        if not mine:
            # Not yours: remember only that, so it is not checked again until it changes.
            counts["not mine"] += 1
            w.update(excluded="not mine", words=0, text_hash="", family=prev["family"] if prev else wid)
            w["meta"] = {k: v for k, v in w.items() if k not in store.WORK_COLS}
            store.save_work(db, w, sig, "")
            continue
        if unchanged and prev["excluded"] in ("internal", "job application", "not mine") and not name_reason:
            unchanged = False  # its text was never read; read it now
        text = None
        if name_reason:
            w.update(excluded=name_reason, words=0, text_hash="")
            text = ""
            counts["excluded"] += 1
        elif unchanged:
            w.update(excluded=prev["excluded"], words=prev["words"], text_hash=prev["text_hash"])
            counts["unchanged"] += 1
        else:
            print(f"  [{i}/{len(files)}] reading {w['path']}", file=sys.stderr)
            try:
                text = drive.text_of(svc, f)
            except Exception as e:
                print(f"    could not read: {e}", file=sys.stderr)
                text = ""
            if w["kind"] == "slides":
                text = scanner.clean_slides(text)
            w["words"] = len(text.split())
            w["text_hash"] = hashlib.sha1(text.encode()).hexdigest()
            w["excluded"] = "too short" if w["words"] < drive.MIN_WORDS else ""
            if w["excluded"]:
                text = ""  # keep no text for what stays out of the catalog
                counts["excluded"] += 1
            else:
                counts["read"] += 1
        w["family"] = prev["family"] if prev else wid
        w["meta"] = {k: v for k, v in w.items() if k not in store.WORK_COLS}
        store.save_work(db, w, sig, text)
        if i % 50 == 0:
            db.commit()
    store.mark_missing(db, "gdrive", seen)
    db.commit()
    print(f"Google Drive: {len(files)} files; {counts['read']} read, {counts['unchanged']} unchanged, "
          f"{counts['excluded']} excluded by title, folder or length, {counts['not mine']} not yours.")


# ---------- output ----------

def family_size(db, w):
    return db.execute("SELECT COUNT(*) FROM works WHERE family = ? AND missing = 0", (w["family"],)).fetchone()[0]


def line(db, w, width=58):
    size = family_size(db, w)
    fam = f"+{size - 1}" if size > 1 else ""
    k = "" if w["koene"] else "?"
    mark = STATE_MARK.get(w["state"], " ") if not w["excluded"] and w["family"] == w["id"] else " "
    return (f"{w['n']:4d} {mark} {w['date'][:7]} {w['kind']:<9} {w['pages']:3d}p {fam:>3}{k:1} "
            f"{store.display_title(w)[:width]}  [{w['path']}]")


def resolve(db, keys):
    out = []
    for k in keys:
        w = store.work(db, k)
        if not w:
            sys.exit(f"no work {k}")
        out.append(w)
    return out


def cmd_list(db, args):
    where = ["missing = 0", "excluded != ''" if args.excluded else "excluded = ''"]
    if not args.all:
        where.append("family = id")
    params = []
    if args.kind:
        where.append("kind = ?")
        params.append(args.kind)
    if args.state:
        where.append("state = ?")
        params.append(args.state)
    elif not args.excluded:
        where.append(store.NOT_ASIDE)
    if args.area:
        where.append("family IN (SELECT work_id FROM work_areas WHERE area = ?)")
        params.append(args.area)
    rows = store.works(db, " AND ".join(where), params)
    if args.sort == "path":
        rows = sorted(rows, key=lambda w: w["path"])
    elif args.sort == "kind":
        rows = sorted(rows, key=lambda w: (w["kind"], w["date"]))
    for w in rows:
        print(line(db, w) + (f"  ({w['excluded']})" if args.excluded else ""))
    print(f"\n{len(rows)} shown. State: N new, A approved, I ignored, S summarized, X set aside "
          "(shown only with --state aside). "
          "'+k' = k other versions; '?' = 'Koene' not on the first pages.")


def cmd_show(db, args):
    w = resolve(db, [args.work])[0]
    meta = json.loads(w["meta"])
    for k in ("n", "id"):
        print(f"{k:>13}: {w[k]}")
    print(f"{'title':>13}: {store.display_title(w)}")
    if store.display_title(w) != w["title"]:
        print(f"{'found title':>13}: {w['title']}")
    for k in ("kind", "state", "excluded", "date", "date_source", "pages", "words"):
        print(f"{k:>13}: {w[k]}")
    keys = ("web_url", "owners", "modified") if w["source"] == "gdrive" else \
        ("tex_date", "docclass", "tex_author", "pdf_author", "has_abstract")
    for k in keys:
        print(f"{k:>13}: {meta.get(k, '')}")
    for f in db.execute("SELECT role, path FROM files WHERE work_id = ?", (w["id"],)):
        print(f"{f['role']:>13}: {f['path']}")
    others = db.execute("SELECT * FROM works WHERE family = ? AND id != ? AND missing = 0 ORDER BY date DESC",
                        (w["family"], w["id"])).fetchall()
    if others:
        print(f"{'versions':>13}:")
        for o in others:
            print(f"{'':>15}{o['n']:4d}  {o['date']}  {o['path']}")
    s = store.summary_of(db, w["family"])
    if s:
        print(f"\n{'summary':>13}: {s['title']}  ({s['date_estimate']}, by {s['by']}, {s['at'][:10]})")
        for p in json.loads(s["key_points"]):
            print(f"{'':>15}- {p}")
        print(f"{'areas':>13}: {', '.join(store.areas_of(db, w['family']))}")
        print(f"{'tags':>13}: {', '.join(store.tags_of(db, w['family']))}")


def cmd_search(db, args):
    rows = store.search(db, " ".join(args.words), args.limit)
    for w in rows:
        # The best hit may be an older version; show the family head with its snippet.
        print(line(db, store.work(db, w["family"]) or w))
        print(f"{'':>10}{' '.join(w['snip'].split())}")
    if not rows:
        print("no matches")


def cmd_stats(db, args):
    live = "missing = 0"
    everything = store.works(db, live)
    works = [w for w in everything if not w["excluded"]]
    heads = [w for w in works if w["family"] == w["id"]]
    reasons = {}
    for w in everything:
        if w["excluded"]:
            reasons[w["excluded"]] = reasons.get(w["excluded"], 0) + 1
    print(f"{len(works)} works, {len(heads)} families; excluded: "
          + (", ".join(f"{n} {r}" for r, n in sorted(reasons.items())) or "none") + "\n")
    print("kind        works  families")
    for k in sorted({w["kind"] for w in works}):
        print(f"{k:<10} {sum(w['kind'] == k for w in works):6d} {sum(w['kind'] == k for w in heads):9d}")
    print("\nstate       families")
    for s in store.STATES:
        print(f"{s:<10} {sum(w['state'] == s for w in heads):9d}")
    print("\narea                        works")
    for a, n in store.area_counts(db).items():
        print(f"{a:<26} {n:6d}")
    print("\ndecade      families")
    dec = {}
    for w in heads:
        d = w["date"][:3] + "0s"
        dec[d] = dec.get(d, 0) + 1
    for d in sorted(dec):
        print(f"{d:<10} {dec[d]:9d}")


def cmd_export(db, args):
    cols = ["n", "head", "state", "excluded", "kind", "date", "pages", "words", "koene", "title", "path"]
    rows = store.works(db, "missing = 0")
    with open(args.file, "w", encoding="utf-8") as f:
        f.write("\t".join(cols) + "\n")
        for w in rows:
            vals = [w["n"], w["family"] == w["id"], w["state"], w["excluded"], w["kind"], w["date"],
                    w["pages"], w["words"], bool(w["koene"]), store.display_title(w), w["path"]]
            f.write("\t".join(str(v).replace("\t", " ") for v in vals) + "\n")
    print(f"wrote {len(rows)} rows to {args.file}")


# ---------- review queue ----------

def cmd_queue(db, args):
    rows = store.queued(db)
    folders = {}
    for w in rows:
        folders.setdefault(store.queue_group(w), []).append(w)
    for folder in sorted(folders, key=str.lower):
        print(f"{store.group_label(folder)}  ({len(folders[folder])})  --folder '{folder}'")
        for w in folders[folder]:
            print("  " + line(db, w))
    print(f"\n{len(rows)} works waiting. approve, ignore or aside them by number, or by --folder prefix.")


def decide(db, args, state):
    try:
        count = store.decide(db, state, args.works, args.folder, args.all_new)
    except ValueError as e:
        sys.exit(str(e))
    db.commit()
    rule = "" if not args.folder else \
        f" Rule for {args.folder} removed." if state == "new" else f" Rule saved for {args.folder}."
    print(f"{count} works now {'set aside' if state == 'aside' else state}.{rule}")


def cmd_rules(db, args):
    for r in db.execute("SELECT * FROM rules ORDER BY prefix"):
        print(f"{r['decision']:<9} {r['prefix']}  ({r['at'][:10]})")


# ---------- summaries ----------

SUMMARY_FORMAT = """\
Write a JSON list, one object per work, to a file, then run
  doctrack.py summarize <file>
Each object:
  {"id": "<work id as given>",
   "title": "<the work's title, cleaned up>",
   "key_points": ["3 to 5 points: the claims, results or proposals, each one sentence"],
   "areas": ["1 or 2 broad areas, exactly as listed below"],
   "tags": ["3 to 8 lowercase specific topic tags, not area names; prefer existing tags"],
   "date_estimate": "YYYY or YYYY-MM, when the work was written or presented"}
"""


def excerpt(text, limit):
    """The start and the end of a long text: abstract and introduction, conclusions."""
    if len(text) <= limit:
        return text
    head = limit * 3 // 4
    return text[:head] + "\n\n[... middle left out ...]\n\n" + text[-(limit - head):]


def cmd_batch(db, args):
    rows = store.works(db, "missing = 0 AND excluded = '' AND family = id AND state = 'approved'")
    if args.works:
        wanted = {w["id"] for w in resolve(db, args.works)}
        rows = [w for w in rows if w["id"] in wanted]
        if len(rows) < len(wanted):
            sys.exit("only approved family heads can be batched")
    rows = rows[:args.n]
    if not rows:
        print("Nothing approved is waiting for a summary.")
        return
    tags = [t["name"] for t in store.tag_counts(db) if not t["merged_into"]]
    print(SUMMARY_FORMAT)
    print("Areas: " + "; ".join(store.AREAS))
    print("Existing tags: " + (", ".join(tags) if tags else "(none yet)") + "\n")
    chars = 0
    for w in rows:
        body = excerpt(store.text_of(db, w["id"]), args.max_chars)
        chars += len(body)
        print(f"=== WORK {w['id']}")
        print(f"kind: {w['kind']} | date: {w['date']} ({w['date_source']}) | pages: {w['pages']} | "
              f"title as found: {w['title']}")
        print(f"--- text ({len(body)} chars)")
        print(body)
        print()
    store.log_send(db, [w["id"] for w in rows], chars, "summarize")
    db.commit()
    print(f"=== {len(rows)} works, {chars} characters (logged).", file=sys.stderr)


def cmd_summarize(db, args):
    with open(args.file, encoding="utf-8") as f:
        items = json.load(f)
    done = 0
    for s in items:
        w = store.work(db, s.get("id", ""))
        if not w:
            print(f"skipped {s.get('id')}: no such work")
        elif w["state"] not in ("approved", "summarized"):
            print(f"skipped {w['id']}: it is {w['state']}, not approved")
        elif not store.was_sent(db, w["id"]):
            print(f"skipped {w['id']}: its text was never handed out by 'batch'")
        elif not 1 <= len(s.get("areas", [])) <= 2 or \
                any(a.strip().lower() not in store.AREAS for a in s["areas"]):
            print(f"skipped {w['id']}: give 1 or 2 areas from: {'; '.join(store.AREAS)}")
        else:
            store.save_summary(db, w["id"], s, args.by)
            done += 1
    db.commit()
    print(f"{done} summaries stored.")


def cmd_sent(db, args):
    for r in db.execute("SELECT * FROM sends ORDER BY id"):
        ids = json.loads(r["work_ids"])
        print(f"{r['at']}  {r['purpose']}  {r['chars']:7d} chars  {len(ids)} works")
        for i in ids:
            print(f"{'':>22}{i}")


def cmd_tags(db, args):
    print("Areas:")
    for a, n in store.area_counts(db).items():
        print(f"{n:4d} {a}")
    print("\nTags:")
    for t in store.tag_counts(db):
        if t["merged_into"]:
            print(f"{'':>4} {t['name']}  -> {t['merged_into']}")
        else:
            print(f"{t['n']:4d} {t['name']}")


def cmd_tag_merge(db, args):
    store.merge_tag(db, args.old.lower(), args.new.lower())
    db.commit()
    print(f"{args.old} merged into {args.new}.")


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("scan")
    s.add_argument("--root", default=DEFAULT_ROOT)
    s.add_argument("--source", choices=["tex", "drive"], help="scan only this source (default: both)")
    sub.add_parser("drive-auth")
    s = sub.add_parser("list")
    s.add_argument("--all", action="store_true", help="include every version, not only family heads")
    s.add_argument("--excluded", action="store_true", help="list the excluded works instead")
    s.add_argument("--kind")
    s.add_argument("--state", choices=store.STATES)
    s.add_argument("--area", choices=store.AREAS)
    s.add_argument("--sort", choices=["date", "path", "kind"], default="date")
    s = sub.add_parser("show")
    s.add_argument("work", help="number from 'list' or work id")
    s = sub.add_parser("search")
    s.add_argument("words", nargs="+", help="FTS5 query, e.g. substrate independent, or \"whole brain\"")
    s.add_argument("--limit", type=int, default=20)
    sub.add_parser("stats")
    s = sub.add_parser("export")
    s.add_argument("file")
    sub.add_parser("queue")
    for name in ("approve", "ignore", "aside", "unreview"):
        s = sub.add_parser(name)
        s.add_argument("works", nargs="*")
        s.add_argument("--folder", help="path prefix below the root, e.g. SFN- or AGI-2010/")
        s.add_argument("--all-new", action="store_true", help="every work now waiting in the queue")
    sub.add_parser("rules")
    s = sub.add_parser("batch")
    s.add_argument("works", nargs="*", help="particular approved works (default: oldest first)")
    s.add_argument("--n", type=int, default=3)
    s.add_argument("--max-chars", type=int, default=24000, help="text per work")
    s = sub.add_parser("summarize")
    s.add_argument("file")
    s.add_argument("--by", default="claude-code-session")
    sub.add_parser("sent")
    sub.add_parser("tags")
    s = sub.add_parser("tag-merge")
    s.add_argument("old")
    s.add_argument("new")
    args = p.parse_args()

    db = store.connect()
    if args.cmd == "scan":
        scan(db, os.path.abspath(os.path.expanduser(args.root)),
             (args.source,) if args.source else ("tex", "drive"))
    elif args.cmd == "drive-auth":
        import drive
        drive.authorize()
        print(f"Drive token saved to {drive.TOKEN_PATH} (scope: drive.readonly).")
    elif args.cmd in ("approve", "ignore", "aside", "unreview"):
        if not (args.works or args.folder or args.all_new):
            sys.exit("give work numbers, --folder or --all-new")
        decide(db, args, {"approve": "approved", "ignore": "ignored", "aside": "aside",
                         "unreview": "new"}[args.cmd])
    else:
        {"list": cmd_list, "show": cmd_show, "search": cmd_search, "stats": cmd_stats,
         "export": cmd_export, "queue": cmd_queue, "rules": cmd_rules, "batch": cmd_batch,
         "summarize": cmd_summarize, "sent": cmd_sent, "tags": cmd_tags,
         "tag-merge": cmd_tag_merge}[args.cmd](db, args)


if __name__ == "__main__":
    main()

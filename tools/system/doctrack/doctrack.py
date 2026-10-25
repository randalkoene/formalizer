#!/usr/bin/env python3
"""doctrack: a catalog of Randal's own writings and presentations.

PROTOTYPE SCANNER. It finds candidate works in ~/doc/tex and lists them for a
check by hand. Nothing is sent anywhere; everything written goes to
doctrack/cache/.

A work, for now, is a main .tex file (one with \\documentclass) that has a
compiled PDF of the same name beside it.

  doctrack.py scan            find works, extract PDF text (incremental)
  doctrack.py list            list works, one line each (family heads only)
  doctrack.py show <n|id>     everything known about one work
  doctrack.py stats           counts by kind, decade and family
  doctrack.py export <file>   write a TSV for review in a spreadsheet

Design: Node 20261015131453.1 and its linked Claude Doc.
"""

import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
from datetime import datetime, timezone

HERE = os.path.dirname(os.path.realpath(__file__))
CACHE_DIR = os.path.join(HERE, "cache")
TEXT_DIR = os.path.join(CACHE_DIR, "text")
WORKS_FILE = os.path.join(CACHE_DIR, "works.json")

DEFAULT_ROOT = os.path.expanduser("~/doc/tex")

# Two works whose PDF texts share at least this fraction of word 5-grams
# (Jaccard) are treated as versions of one another.
FAMILY_THRESHOLD = 0.5
SHINGLE_WORDS = 5

DOCCLASS_RE = re.compile(r"^[^%\n]*\\documentclass\s*(?:\[[^\]]*\])?\s*\{([^}]*)\}", re.M)


# ---------- LaTeX parsing ----------

def strip_comments(tex):
    return re.sub(r"(?<!\\)%.*", "", tex)


def braced_arg(tex, macro):
    """Return the first {...} argument of \\macro, braces balanced, or None."""
    m = re.search(r"\\" + macro + r"\s*(?:\[[^\]]*\])?\s*\{", tex)
    if not m:
        return None
    depth, i = 1, m.end()
    while i < len(tex) and depth:
        if tex[i] == "{":
            depth += 1
        elif tex[i] == "}":
            depth -= 1
        i += 1
    return tex[m.end():i - 1] if depth == 0 else None


def drop_macro_with_arg(s, macro):
    while True:
        arg = braced_arg(s, macro)
        if arg is None:
            return s
        s = s.replace(arg, "", 1)
        s = re.sub(r"\\" + macro + r"\s*(?:\[[^\]]*\])?\s*\{\s*\}", "", s, count=1)


def expand_macros(s, tex):
    """Replace \\foo in s by its \\newcommand{\\foo}{...} or \\def\\foo{...} body from tex."""
    if not s:
        return s
    for name in set(re.findall(r"\\([a-zA-Z@]+)", s)):
        m = re.search(r"\\(?:re)?newcommand\*?\s*\{?\\" + name + r"\}?\s*\{|\\def\\" + name + r"\s*\{", tex)
        if not m:
            continue
        depth, i = 1, m.end()
        while i < len(tex) and depth:
            depth += {"{": 1, "}": -1}.get(tex[i], 0)
            i += 1
        if depth == 0:
            s = re.sub(r"\\" + name + r"(?![a-zA-Z@])", lambda _: tex[m.end():i - 1], s)
    return s


def plain(s):
    """Rough LaTeX to plain text, good enough for titles and author lines."""
    if s is None:
        return ""
    for macro in ("thanks", "footnote", "affiliation", "email", "inst"):
        s = drop_macro_with_arg(s, macro)
    s = s.replace("\\\\", " ").replace("~", " ")
    s = re.sub(r"\\(?:and|And|AND)\b", ",", s)
    s = re.sub(r"\\[a-zA-Z@]+\*?", " ", s)
    s = re.sub(r"[{}$]", "", s)
    s = s.replace("``", '"').replace("''", '"')
    return re.sub(r"\s+", " ", s).strip()


def parse_tex(path):
    with open(path, encoding="utf-8", errors="replace") as f:
        raw = f.read()
    m = DOCCLASS_RE.search(raw)
    tex = strip_comments(raw)
    date = plain(expand_macros(braced_arg(tex, "date"), tex))
    if date.lower() in ("", "today"):
        date = ""
    docclass = m.group(1).strip() if m else ""
    if docclass == "res":
        # In the res class \title is a job title; the document is a CV of \name.
        title = "CV: " + plain(braced_arg(tex, "name"))
    else:
        title = plain(expand_macros(braced_arg(tex, "title"), tex))
    return {
        "docclass": docclass,
        "tex_title": title,
        "tex_author": plain(expand_macros(braced_arg(tex, "author"), tex)),
        "tex_date": date,
        "has_abstract": bool(re.search(r"\\begin\{abstract\}|\\abstract\b", tex)),
        "is_letter": bool(re.search(r"\\begin\{letter\}", tex)),
    }


# ---------- PDF ----------

def run(cmd):
    try:
        return subprocess.run(cmd, capture_output=True, text=True, errors="replace",
                              timeout=120).stdout
    except (subprocess.TimeoutExpired, OSError):
        return ""


def pdf_info(path):
    info = {}
    for line in run(["pdfinfo", path]).splitlines():
        k, _, v = line.partition(":")
        info[k.strip()] = v.strip()
    return info


def pdf_date(info):
    """CreationDate from pdfinfo as YYYY-MM-DD, or ''."""
    v = info.get("CreationDate", "")
    for fmt in ("%a %b %d %H:%M:%S %Y %Z", "%a %b %d %H:%M:%S %Y"):
        try:
            return datetime.strptime(v, fmt).strftime("%Y-%m-%d")
        except ValueError:
            pass
    return ""


def text_file(work_id):
    h = hashlib.sha1(work_id.encode()).hexdigest()[:16]
    return os.path.join(TEXT_DIR, h + ".txt")


# ---------- classification ----------

def guess_kind(w):
    path = w["id"].lower()
    stem = os.path.basename(path)
    cls = w["docclass"].lower()
    if cls in ("beamer", "foils") or "slides" in stem or "presentation" in path:
        return "slides"
    if "poster" in path:
        return "poster"
    if cls == "res" or re.search(r"(^|[-_.])(cv|resume)([-_.]|$)", stem):
        return "cv"
    if "statement" in stem:
        return "statement"
    if w["is_letter"] or cls == "letter" or "letter" in stem:
        return "letter"
    if "abstract" in stem and w["pages"] <= 2:
        return "abstract"
    if "report" in path:
        return "report"
    if "proposal" in path or "grant" in path:
        return "proposal"
    return "document"


def exclusion(w):
    """Why a work stays out of the catalog, or '' when it belongs in it."""
    if w["id"].startswith("cv/") or w["kind"] in ("cv", "statement", "letter"):
        return "job application"
    if not w["koene"] and re.search(r"sample|template|instruction|typesetting|ws-ijmc|IOS-Book",
                                    w["title"] + " " + w["id"], re.I):
        return "publisher template"
    return ""


def best_date(w):
    """(YYYY-MM-DD, source). PDF creation date beats file time; \\date is kept separately."""
    if w["pdf_created"]:
        return w["pdf_created"], "pdf"
    t = datetime.fromtimestamp(w["pdf_mtime"], tz=timezone.utc)
    return t.strftime("%Y-%m-%d"), "mtime"


def best_title(w):
    if w["tex_title"]:
        return w["tex_title"]
    if w["pdf_title"] and not re.search(r"\.(dvi|tex)$|^untitled$|^microsoft word", w["pdf_title"], re.I):
        return w["pdf_title"]
    return w.get("first_line", "") or os.path.basename(w["id"])


# ---------- families (near-duplicates) ----------

def shingles(text):
    words = re.findall(r"[a-z0-9]+", text.lower())
    n = SHINGLE_WORDS
    return {hash(" ".join(words[i:i + n])) for i in range(max(0, len(words) - n + 1))}


def assign_families(works):
    sh = {}
    for w in works:
        try:
            with open(text_file(w["id"]), encoding="utf-8", errors="replace") as f:
                sh[w["id"]] = shingles(f.read())
        except OSError:
            sh[w["id"]] = set()
    parent = {w["id"]: w["id"] for w in works}

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    ids = [w["id"] for w in works if len(sh[w["id"]]) >= 20]
    for i, a in enumerate(ids):
        sa = sh[a]
        for b in ids[i + 1:]:
            sb = sh[b]
            small, large = (sa, sb) if len(sa) <= len(sb) else (sb, sa)
            if len(small) < FAMILY_THRESHOLD * len(large):
                continue  # Jaccard cannot reach the threshold
            inter = len(small & large)
            if inter / (len(sa) + len(sb) - inter) >= FAMILY_THRESHOLD:
                parent[find(a)] = find(b)

    groups = {}
    for w in works:
        groups.setdefault(find(w["id"]), []).append(w)
    for members in groups.values():
        members.sort(key=lambda w: w["date"], reverse=True)
        head = members[0]["id"]
        for w in members:
            w["family"] = head
            w["family_size"] = len(members)


# ---------- scan ----------

def find_mains(root):
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames.sort()
        for fn in sorted(filenames):
            if not fn.endswith(".tex"):
                continue
            tex = os.path.join(dirpath, fn)
            pdf = tex[:-4] + ".pdf"
            if not os.path.isfile(pdf):
                continue
            try:
                with open(tex, encoding="utf-8", errors="replace") as f:
                    if not DOCCLASS_RE.search(f.read()):
                        continue
            except OSError:
                continue
            yield tex, pdf


def load_works():
    try:
        with open(WORKS_FILE) as f:
            return json.load(f)
    except (OSError, ValueError):
        return {"root": None, "works": []}


def scan(root):
    os.makedirs(TEXT_DIR, exist_ok=True)
    old = {w["id"]: w for w in load_works()["works"]}
    works, reused = [], 0
    for tex, pdf in find_mains(root):
        wid = os.path.relpath(tex, root)[:-4]
        ts, ps = os.stat(tex), os.stat(pdf)
        sig = [ts.st_mtime, ts.st_size, ps.st_mtime, ps.st_size]
        prev = old.get(wid)
        if prev and prev.get("sig") == sig and os.path.isfile(text_file(wid)):
            prev.update(parse_tex(tex))  # cheap, and picks up parser improvements
            works.append(prev)
            reused += 1
            continue
        print(f"  reading {wid}", file=sys.stderr)
        w = {"id": wid, "tex": tex, "pdf": pdf, "sig": sig, "pdf_mtime": ps.st_mtime}
        w.update(parse_tex(tex))
        info = pdf_info(pdf)
        w["pages"] = int(info.get("Pages", "0") or 0)
        w["pdf_title"] = info.get("Title", "")
        w["pdf_author"] = info.get("Author", "")
        w["pdf_created"] = pdf_date(info)
        text = run(["pdftotext", "-enc", "UTF-8", pdf, "-"])
        with open(text_file(wid), "w", encoding="utf-8") as f:
            f.write(text)
        w["words"] = len(text.split())
        first = run(["pdftotext", "-l", "2", pdf, "-"])
        w["first_line"] = next((l.strip() for l in first.splitlines() if len(l.strip()) > 3), "")
        w["koene"] = bool(re.search(r"koene", first + w["tex_author"] + w["pdf_author"], re.I))
        works.append(w)
    for w in works:
        w["kind"] = guess_kind(w)
        w["date"], w["date_source"] = best_date(w)
        w["title"] = best_title(w)
        w["excluded"] = exclusion(w)
    assign_families(works)
    works.sort(key=lambda w: (w["date"], w["id"]))
    with open(WORKS_FILE, "w") as f:
        json.dump({"root": root, "scanned": datetime.now().isoformat(timespec="seconds"),
                   "works": works}, f, indent=1)
    heads = sum(1 for w in works if w["family"] == w["id"])
    print(f"{len(works)} works ({reused} unchanged), {heads} families. Saved {WORKS_FILE}")


# ---------- output ----------

def numbered(works):
    return list(enumerate(works, 1))


def line(n, w, width=60):
    fam = f"+{w['family_size'] - 1}" if w["family_size"] > 1 else ""
    k = "" if w["koene"] else "?"
    return (f"{n:4d} {w['date'][:7]} {w['kind']:<9} {w['pages']:3d}p {fam:>3}{k:1} "
            f"{w['title'][:width]}  [{w['id']}]")


def cmd_list(args):
    data = load_works()
    rows = numbered(data["works"])
    rows = [(n, w) for n, w in rows if bool(w["excluded"]) == args.excluded]
    if not args.all:
        rows = [(n, w) for n, w in rows if w["family"] == w["id"]]
    if args.kind:
        rows = [(n, w) for n, w in rows if w["kind"] == args.kind]
    if args.sort == "path":
        rows.sort(key=lambda r: r[1]["id"])
    elif args.sort == "kind":
        rows.sort(key=lambda r: (r[1]["kind"], r[1]["date"]))
    for n, w in rows:
        print(line(n, w) + (f"  ({w['excluded']})" if args.excluded else ""))
    print(f"\n{len(rows)} shown. '+k' = k other versions; '?' = 'Koene' not found in the first pages.")


def cmd_show(args):
    works = load_works()["works"]
    key = args.work
    hit = works[int(key) - 1] if key.isdigit() and 0 < int(key) <= len(works) else \
        next((w for w in works if w["id"] == key), None)
    if not hit:
        sys.exit(f"no work {key}")
    for k in ("id", "title", "kind", "excluded", "date", "date_source", "tex_date", "docclass", "pages",
              "words", "koene", "tex_author", "pdf_author", "has_abstract", "tex", "pdf"):
        print(f"{k:>13}: {hit[k]}")
    fam = [w for w in works if w["family"] == hit["family"] and w is not hit]
    if fam:
        print(f"{'versions':>13}:")
        for w in fam:
            print(f"{'':>15}{w['date']}  {w['id']}")


def cmd_stats(args):
    everything = load_works()["works"]
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
    print("\ndecade      families")
    dec = {}
    for w in heads:
        d = w["date"][:3] + "0s"
        dec[d] = dec.get(d, 0) + 1
    for d in sorted(dec):
        print(f"{d:<10} {dec[d]:9d}")
    print(f"\n'Koene' not found in first pages: {sum(not w['koene'] for w in heads)} families")


def cmd_export(args):
    works = load_works()["works"]
    cols = ["n", "head", "excluded", "kind", "date", "pages", "words", "koene", "family_size", "title", "id"]
    with open(args.file, "w", encoding="utf-8") as f:
        f.write("\t".join(cols) + "\n")
        for n, w in numbered(works):
            row = [n, w["family"] == w["id"], w["excluded"], w["kind"], w["date"], w["pages"], w["words"],
                   w["koene"], w["family_size"], w["title"], w["id"]]
            f.write("\t".join(str(v).replace("\t", " ") for v in row) + "\n")
    print(f"wrote {len(works)} rows to {args.file}")


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("scan")
    s.add_argument("--root", default=DEFAULT_ROOT)
    s = sub.add_parser("list")
    s.add_argument("--all", action="store_true", help="include every version, not only family heads")
    s.add_argument("--excluded", action="store_true", help="list the excluded works instead")
    s.add_argument("--kind")
    s.add_argument("--sort", choices=["date", "path", "kind"], default="date")
    s = sub.add_parser("show")
    s.add_argument("work", help="number from 'list' or work id")
    sub.add_parser("stats")
    s = sub.add_parser("export")
    s.add_argument("file")
    args = p.parse_args()
    if args.cmd == "scan":
        scan(os.path.abspath(os.path.expanduser(args.root)))
    else:
        {"list": cmd_list, "show": cmd_show, "stats": cmd_stats, "export": cmd_export}[args.cmd](args)


if __name__ == "__main__":
    main()

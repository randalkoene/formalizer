"""Finding works in a LaTeX tree and reading what they are.

A work, for now, is a main .tex file (one with \\documentclass) that has a
compiled PDF of the same name beside it. Nothing here writes anywhere; the
caller stores the results.
"""

import os
import re
import subprocess
from datetime import datetime, timezone

# Two works whose PDF texts share at least this fraction of word 5-grams
# (Jaccard) are treated as versions of one another.
FAMILY_THRESHOLD = 0.5
SHINGLE_WORDS = 5

# Raise when text extraction changes, so that the next scan re-reads every PDF.
# 2: slide decks lose repeated lines and Beamer build-up pages.
TEXT_VERSION = 2

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


def read_pdf(pdf, tex_author):
    """PDF facts plus the full extracted text (returned separately: it is large)."""
    info = pdf_info(pdf)
    text = run(["pdftotext", "-enc", "UTF-8", pdf, "-"])
    first = run(["pdftotext", "-l", "2", pdf, "-"])
    facts = {
        "pages": int(info.get("Pages", "0") or 0),
        "pdf_title": info.get("Title", ""),
        "pdf_author": info.get("Author", ""),
        "pdf_created": pdf_date(info),
        "pdf_mtime": os.stat(pdf).st_mtime,
        "words": len(text.split()),
        "first_line": next((l.strip() for l in first.splitlines() if len(l.strip()) > 3), ""),
        "koene": bool(re.search(r"koene", first + tex_author + info.get("Author", ""), re.I)),
    }
    return facts, text


def clean_slides(text):
    """Text of a slide deck without its repetitions.

    pdftotext separates pages with form feeds. Lines printed on most pages
    (navigation bars, headers, footers) are dropped, and so is a page whose
    remaining lines all reappear on the next page: a Beamer build step whose
    final state follows.
    """
    pages = [[l.strip() for l in p.splitlines() if l.strip()] for p in text.split("\f")]
    pages = [p for p in pages if p]
    if len(pages) < 4:
        return text
    seen_on = {}
    for p in pages:
        for l in set(p):
            seen_on[l] = seen_on.get(l, 0) + 1
    common = {l for l, n in seen_on.items() if n > len(pages) / 2}
    pages = [[l for l in p if l not in common] for p in pages]
    kept = [p for i, p in enumerate(pages)
            if p and not (i + 1 < len(pages) and set(p) <= set(pages[i + 1]))]
    return "\n\f\n".join("\n".join(p) for p in kept)


# ---------- classification ----------

def guess_kind(w):
    path = w["path"].lower()
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
    if w["path"].startswith("cv/") or w["kind"] in ("cv", "statement", "letter"):
        return "job application"
    if not w["koene"] and re.search(r"sample|template|instruction|typesetting|ws-ijmc|IOS-Book",
                                    w["title"] + " " + w["path"], re.I):
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
    return w.get("first_line", "") or os.path.basename(w["path"])


def classify(w):
    """Fill in kind, date, title and exclusion from the raw facts in w."""
    w["kind"] = guess_kind(w)
    w["date"], w["date_source"] = best_date(w)
    w["title"] = best_title(w)
    w["excluded"] = exclusion(w)


# ---------- families (near-duplicates) ----------

def shingles(text):
    words = re.findall(r"[a-z0-9]+", text.lower())
    n = SHINGLE_WORDS
    return {hash(" ".join(words[i:i + n])) for i in range(max(0, len(words) - n + 1))}


def families(works, texts):
    """Map work id -> id of its family head (the newest version).

    works: dicts with 'id' and 'date'; texts: work id -> extracted text.
    """
    sh = {w["id"]: shingles(texts.get(w["id"], "")) for w in works}
    parent = {w["id"]: w["id"] for w in works}

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    # Candidate pairs share at least 3 sampled 5-grams; only those are compared
    # in full. 5-grams found in very many works (letterheads, boilerplate) are
    # no evidence and are skipped.
    ids = [w["id"] for w in works if len(sh[w["id"]]) >= 20]
    index = {}
    for wid in ids:
        for s in sh[wid]:
            if s % 8 == 0:
                index.setdefault(s, []).append(wid)
    shared = {}
    for bucket in index.values():
        if 1 < len(bucket) <= 50:
            for i, a in enumerate(bucket):
                for b in bucket[i + 1:]:
                    shared[(a, b)] = shared.get((a, b), 0) + 1
    for (a, b), n in shared.items():
        if n >= 3:
            sa, sb = sh[a], sh[b]
            small, large = (sa, sb) if len(sa) <= len(sb) else (sb, sa)
            if len(small) < FAMILY_THRESHOLD * len(large):
                continue  # Jaccard cannot reach the threshold
            inter = len(small & large)
            if inter / (len(sa) + len(sb) - inter) >= FAMILY_THRESHOLD:
                parent[find(a)] = find(b)

    groups = {}
    for w in works:
        groups.setdefault(find(w["id"]), []).append(w)
    head = {}
    for members in groups.values():
        newest = max(members, key=lambda w: (w["date"], w["id"]))["id"]
        for w in members:
            head[w["id"]] = newest
    return head


# ---------- finding works ----------

def find_mains(root):
    """Yield (tex, pdf) for every main .tex below root with a PDF beside it."""
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

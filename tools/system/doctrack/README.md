# doctrack

A catalog of Randal's own writings and presentations, by topic and date.
Design and open questions: Node 20261015131453.1 and its linked Claude Doc.

**Status:** scanner for `~/doc/tex`, SQLite store with keyword search, review
queue, summaries written in a Claude Code session, and a browser UI. Google
Drive and the CV come next.

Everything is stored in `cache/doctrack.db` (gitignored). Set `DOCTRACK_DB` to
use another database file, e.g. for testing.

## Files

- `doctrack.py`: the command line
- `scanner.py`: finds works in a LaTeX tree, reads titles, dates and PDF text,
  guesses kind and exclusions, groups versions into families
- `store.py`: the SQLite schema and all database access
- `serve.py` and `web/`: the browser UI on http://127.0.0.1:8713
- `doctrack-start.py`, `doctrack-cgi.py`: start the UI from the dashboard

## Browser UI

```
./serve.py                 # opens http://127.0.0.1:8713 in your browser
./serve.py --status
./serve.py --stop
./serve.py --idle-exit 180 # quit after 3 hours without a request
```

From the Formalizer dashboard, the **Doc Track** button (Information) starts the
server if needed and opens it. The button goes to `doctrack-cgi.py`, which asks
fzserverpq to run `doctrack-start.py` as you via `CGIbg_run_as_user` (it must be
listed in `predefined_CGIbg`). Started that way, the server runs detached, logs
to `cache/serve.log`, and exits after 180 idle minutes. The **⏻** button stops it.

- **Queue:** works waiting for review, grouped by folder. Approve or ignore one
  work, or a whole folder (saved as a rule for works found there later), or
  approve everything waiting. Each row links to its PDF.
- **Catalog:** full-text search with highlighted snippets, and filters by
  state, kind, decade and tag. Shows versions and excluded works on request.
- **Work page:** summary and tags, versions, files, PDF, and the queue actions.
- **Tags:** the vocabulary with counts; merge one tag into another.
- **Sent:** every batch of text handed out for summarizing.
- **Scan** re-scans `~/doc/tex` in the background.

The server listens on 127.0.0.1 only, refuses POSTs from other sites, and serves
a PDF only when it belongs to a catalogued work and lies under `~/doc/tex`.
Summarizing is not done from the UI: it stays in a Claude Code session.

## Commands

```
./doctrack.py scan                   # find works in ~/doc/tex (incremental)
./doctrack.py list                   # one line per family head, oldest first
./doctrack.py list --state new --kind slides
./doctrack.py list --excluded        # what is left out, and why
./doctrack.py show 238               # everything about work 238, summary included
./doctrack.py search '"whole brain" emulation'
./doctrack.py stats
./doctrack.py export works.tsv

./doctrack.py queue                  # works waiting for a decision, by folder
./doctrack.py approve 4 5 6          # may be summarized by Claude
./doctrack.py approve --folder SFN-  # every work below SFN-*, now and later
./doctrack.py ignore 13              # searchable, never summarized
./doctrack.py unreview 13            # back to the queue
./doctrack.py rules                  # remembered folder decisions

./doctrack.py batch --n 3            # print approved works for summarizing
./doctrack.py summarize sums.json    # store what Claude wrote
./doctrack.py sent                   # every hand-out of text, with time and size
./doctrack.py tags
./doctrack.py tag-merge connectomics connectome
```

Numbers are assigned once per work and never reused, so they stay valid across
scans.

## What counts as a work

A main `.tex` file (one with `\documentclass`, outside a comment) with a
compiled PDF of the same name beside it. Its id is `tex:` plus its path below
the root, without `.tex`.

- **Title:** `\title` with macros defined in the same file expanded, else the PDF
  Title, else the first line of the PDF text. For the `res` class, `CV: \name`.
- **Date:** the PDF creation date, else the PDF file time. A `\date` is kept but
  not trusted.
- **Kind:** slides, poster, cv, statement, letter, abstract, report, proposal or
  document, guessed from the document class and the path.
- **Koene:** whether "Koene" appears in the first two PDF pages or the author
  fields. `?` in `list` marks the ones without.
- **Families:** works whose PDF texts share at least half their word 5-grams
  are versions of one another. The newest is the family head. Only heads are
  reviewed and summarized; `list --all` shows the other versions too.
- **Excluded:** job application material (everything under `cv/`, and CVs,
  statements and cover letters elsewhere) and publisher templates. Excluded
  works stay in the store with the reason.

A re-scan re-reads every `.tex` (cheap), but runs `pdfinfo` and `pdftotext` only
for works whose `.tex` or PDF changed in size or modification time. Works that
disappear are marked missing, not deleted.

## The review queue and what leaves this machine

Each family head has a state: `new` (waiting for review), `approved`, `ignored`
or `summarized`. Folder decisions are saved as rules, so works that appear later
under that folder get the same state without asking.

Document text leaves the machine only through `batch`, which prints the text
of approved works, at most `--max-chars` (24000) per work: the start and the end
of a long text. Each call is logged; `sent` shows the log. `summarize` accepts
summaries only for works that are approved and were handed out by `batch`.

When the text of a summarized work changes, it goes back to `approved`, so the
next `batch` picks it up again.

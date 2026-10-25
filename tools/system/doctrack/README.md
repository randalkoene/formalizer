# doctrack

A catalog of Randal's own writings and presentations, by topic and date.
Design and open questions: Node 20261015131453.1 and its linked Claude Doc.

**Status: prototype scanner.** It finds candidate works in `~/doc/tex` and lists
them for a check by hand. Nothing is sent anywhere. Everything it writes goes to
`cache/` (gitignored): `works.json` and the extracted PDF text in `cache/text/`.

```
./doctrack.py scan              # find works, extract PDF text (incremental)
./doctrack.py list              # one line per family head, oldest first
./doctrack.py list --all        # every version too
./doctrack.py list --kind slides --sort path
./doctrack.py show 238          # everything known about work 238 (number from list)
./doctrack.py stats             # counts by kind, decade and family
./doctrack.py export works.tsv  # TSV for review in a spreadsheet
```

## What counts as a work

A main `.tex` file (one with `\documentclass`, outside a comment) with a compiled
PDF of the same name beside it. The work id is its path below the root, without
`.tex`.

## What the scanner works out

- **Title:** `\title` with macros defined in the same file expanded, else the PDF
  Title, else the first line of the PDF text. For the `res` class, `CV: \name`.
- **Date:** the PDF creation date, else the PDF file time. A `\date` is kept as
  `tex_date` but not trusted.
- **Kind:** slides, poster, cv, statement, letter, abstract, report, proposal or
  document, guessed from the document class and the path.
- **Koene:** whether "Koene" appears in the first two PDF pages or the author
  fields. `?` in `list` marks the ones without, mostly templates and samples.
- **Families:** works whose PDF texts share at least half their word 5-grams
  are versions of one another. The newest is the family head; `list` shows only
  heads unless `--all`.

- **Excluded:** job application material (everything under `cv/`, and CVs,
  statements and cover letters elsewhere) and publisher templates. Excluded
  works stay in `works.json` with the reason; `list --excluded` shows them.

A re-scan re-reads every `.tex` (cheap), but runs `pdfinfo` and `pdftotext` only
for works whose `.tex` or PDF changed in size or modification time.

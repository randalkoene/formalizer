# fzguide.system - Authoritative server of System guide content.

This Formalizer environment server program provides a target independent authoritative source
for content in the System guide. The server program `fzguide.system` carries out interactions
with the underlying database to retrieve guide content or store new guide content.

Examples are:

- The AM System Morning protocol guide.
- The PM System Evening protocol guide.


--------

The `fzguide.system-cgi.py` tool is provided as a CGI handler, together with the
`fzguide.system-form.html` HTML page, as an alternative but straightforward web interface
to the `fzguide.system` server.

The server, the CGI handler, and the HTML page can all be installed from the Formalizer
source code root by running `make` on the `executables` target. For example:

```
    cd ~/src/formalizer
    make executables
```

--------

The `system-guide.20200912-flat.html` file is a snapshot flat HTML file created by removing unnecessary page formatting bulk from the v1.x iteration of the authoritative System Summary (which was found at `~/doc/tex/Change/NoC-System-Summary.html`). Its contents form the initial individual entries for the guide sections in the database.

The `example_snippet.txt` file shows in rudimentary form what a single snippet (entry) can look like.

--------

### Snippet IDs

Snippets are stored in the `<schema>.guide_system` table, with the snippet ID as primary key. An ID has five parts separated by colons:

```
{chapter_label}:{section_number}:{subsection_number}:{subsection_label}:{index_number}
```

- `chapter_label`: `am`, `act` or `pm` (selected on the command line with `-A`, `-T` or `-P`).
- `section_number`, `subsection_number`, `index_number`: decimal numbers > 0.0, written like `01.1`.
- `subsection_label`: a text label, e.g. `wakeup`. The label `SEC` marks an entry that holds a section header; its subsection and index numbers are presented as `01.1`.

For example: `am:01.1:01.1:SEC:01.1`.

### Adding many snippets from one file

To store many snippets at once, prepare a file in this format:

- The file must begin with `<!-- FZGUIDE -->`.
- Snippets are read only between `<!-- begin: content -->` (followed by a newline) and `<!-- end  : content -->` (note the two spaces before the colon).
- Each snippet starts with an ID line such as `<!-- ID={am:01.1:01.1:SEC:01.1} -->`. The snippet content runs from the next line up to the next ID line, or up to the end marker.

Then store it with, for example:

```
fzguide.system -S -A -i system-guide.20200925-flat.html
```

When the file begins with `<!-- FZGUIDE -->`, the IDs in the file are used, so the chapter option and the other ID options on the command line do not determine where the snippets go. Empty snippets are skipped with a warning. The snippets are added with plain `INSERT` commands, so a snippet whose ID already exists in the table makes the import fail at that point. To replace the whole guide, the table can be dropped with `fzsetup.py -R guide` first (a new table is created when `fzguide.system` next stores a snippet); make sure the content still exists elsewhere before doing that.

The `system-guide.*-flat.html` files in this directory use this format.

Source: Trello card MV6hRktm (archived in ~/.formalizer/archive/trello/cards/).

---------------


Randal A. Koene, 2020

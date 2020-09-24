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

---------------


Randal A. Koene, 2020

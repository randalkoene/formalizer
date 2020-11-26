# fzlogtime - Command line and CGI tool to build a time-picker HTML page to call fztask.

Generates an HTML page showing the day in 5 minute steps. Times later than the most
recent Log chunk are selectable. Clicking on of the links calls `fztask` via `fztask-cgi.py`
with an emulated time specified by the time-specific link.

A link for the Actual Time is also provided, as is a FORM input for arbitrary date
time selection.

Note that CGI calls can be fickle. For this reason, the `-C` option from the command line,
or the `cgivar=` argument in a GET or POST CGI call can enable variants.

Here are some examples of `fzlogtime` calls that work from the command line or as CGI
calls from different browsers:

Example 1: Launching into an X-terminal from the command line.

```
    fzlogtime-term.sh
```

Example 2: CGI call from w3m via local HTTP server from the command line.

```
    w3m http://localhost/cgi-bin/fzlogtime.cgi?cgivar=wrap
```

Example 3: CGI call from w3m without HTTP server from the command line.

```
    w3m /cgi-bin/fzlogtime.cgi?cgivar=wrap
```

Example 4: CGI call from Chrome via HTTP server at IP address aether.local, in the URL bar,
or as launch argument from another host or virtual machine.

```
    http://aether.local/cgi-bin/fzlogtime?cgivar=skip&
```

Example 5: Alternative CGI call from Chrome via HTTP server at IP address aether.local, in
the URL bar, or as launch argument.

```
    http://aether.local/cgi-bin/fzlogtime.cgi?cgivar=wrap&D=20201124
```

Notes:

- The variant with `fzlogtime.cgi?cgivar=wrap,skip` does not work on Chrome.
- No unwrapped variant works on w3m.
- Firefox behaves the same way as Chrome.

Command line calls to `fzloghtml` can also be used to generate the HTML output in `(cmd)`
mode instead of `(cgi)` mode. The resuting text stream can be sent to a file or STDOUT, and
from there it can be used as an initial static page to show in a browser. To enable any
needed CGI variants for links in the generated page, the variants are also available as
command line arguments with the `-C` option.

Randal A. Koene, 2020

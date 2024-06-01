# `fzgraphhtml` - Generate formatted Graph data output

The `fzgraphhtml` tool is used to query the Graph and produce formatted output with the requested information.

## (( ... add documentation about direct calls... ))

## The `fzgraphhtml-cgi.py` CGI script

Call modes are:

- **help** (form variable `help=`): Shows information about the CGI script as an HTML page.
- **id** (form variable `id=`): Generate an HTML embeddable list of NNLs.
- **srclist** (form variable `srclist=<list_name>`): Generate an HTML page for the NNL `<list_name>`.
- **edit** (form variable `edit=<node-id>|new`): Generate a Node Edit HTML page for Node `<node-id>` for a Node Create HTML page.
- **topicslist** (form variable `topics=`): Generate an HTML page showing all defined `topics`.
- **topics_alt** (form variable `topics_alt=`): Use a custom template to generate a page of `topics`.
- **topic** (form variable `topic=<topic>`): Generate an HTML page showing the Nodes in `<topic>`.
- **norepeats** (form variable `norepeats=`): Generate an HTML list of incomplete Nodes.
- **default**: Generate an HTML page showing the Next Nodes Schedule.

### Generating an HTML page for Node Editing

This uses `fzgraphhtml -m <node-id>`. This calls the `render_node_edit()` function in `fzgraphhtml/render.cpp`.

### Generating an HTML page for Node Creation

This uses `fzgraph -q -C "/fz/graph/namedlists/superiors?add={tosup}&unique=true"` with the `tosup` variable
or `fzgraph -q -C "/fz/graph/namedlists/dependencies?add={todep}&unique=true"` with the `todep` variable,
and then uses `fzgraphhtml -m new`. This called the `render_new_node_page()` function in `fzgraphhtml/render.cpp`.

---
Randal A. Koene, 2020

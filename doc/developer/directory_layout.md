# Directory layout and program names

This describes how the source tree at `~/src/formalizer` is organized, and the
reasoning behind the names and the division of work between core programs.

## Top level

```
~/src/formalizer
  config/     default configuration files, one directory per component
  core/       core components (see core/README.md for what belongs there)
  doc/        documentation (this directory, the manual, Doxygen output)
  inactive/   older code, not referenced by the Makefiles
  media/      images used in documentation
  tools/      everything that is not core
  Makefile    build and deploy targets (build, executables, doxygen, ...)
```

## core

```
core/include          public C++ headers of the core library, and shared Python modules
core/lib              C++ sources of the core library (built into object files used by all programs)
core/doc              core design notes (e.g. schedule-updating.md)
core/fzbackup         back up the database and the environment
core/fzedit           edit parameters of Nodes and Edges
core/fzgraph          add Nodes and Edges, modify Named Node Lists, send server API requests
core/fzgraphsearch    search the Graph
core/fzguide.system   System guide content
core/fzinfo           information about the installed environment
core/fzlog            modify the Log (open and close chunks, add entries)
core/fzmetricspq      access to the Metrics tables
core/fzquerypq        single queries to the database (no cache, no server)
core/fzserverpq       the memory-resident Graph data server
core/fzserverpq-log   a Log data server (a stub, not built or installed; see architecture.md)
core/fzsetup          set up or reset a Formalizer environment
core/fztask           task control loop
core/fztask-server    Flask server-sent events (SSE) helper for fztask
core/fztimezone       time zone adjustments
core/fzupdate         Schedule updating
```

Public headers that many programs use go in `core/include`. Headers that belong
to one program stay in that program's directory (all C++ headers use `.hpp`).

## tools

```
tools/compat       compatibility scripts from the transition from Formalizer 1.x (dil2al)
tools/conversion   format conversion (dil2graph: 1.x to 2.x, graph2dil: 2.x to 1.x)
tools/dev          development tools (boilerplate, fzbuild, fzpackage)
tools/glue         automation and scheduled jobs (fzcrontab, fzdaily.sh, fzhourly.sh)
tools/interface    interfaces: HTML output generators, CGI scripts, command line front-ends
                   (fzgraphhtml, fzloghtml, fzlogtime, fzlink, nodeboard, addnode,
                   logentry, fzdashboard, fzserver-info, ...), and shared web
                   resources in tools/interface/resources
tools/misc         tests and small experiments
tools/system       tools for System protocols (daywiz, schedule, metrics, planning,
                   replicate, top dashboards, fzclaude, ...)
```

The [manual](../manual/manual.md) (Appendix B) lists the programs with links to
their READMEs.

## Program names

Program names start with `fz` (or are otherwise unlikely to collide with other
programs), e.g. `fzserverpq` rather than `server`, `fztask` rather than `task`,
`fzupdate` rather than `update`, `fzlog` rather than `log`.

The names are short: `fzgraph`, not `fzgraphedit`. The core programs are divided
by the kind of change they make, not by the kind of Graph element:

- **`fzgraph` changes the structure of the Graph:** it adds Nodes (`-M node`) and
  Edges (`-M edges`), and modifies Named Node Lists (`-L add|remove|delete`). It
  can also pass any request to the server port API (`-C`).
- **`fzedit` changes parameters inside existing elements:** all edits of Node and
  Edge parameters go through `fzedit -M <node-id>|<edge-id>`, including which
  Topics a Node belongs to (`-g`).

There is no `fznode`, `fzedge` or `fztopic`.

Rationale for where Topics and Named Node Lists fit:

- A Topic is a set that Nodes belong to (with a relevance value). Adding or
  removing Topics changes the structure of the Graph, in the sense that set
  membership pulls Nodes together. Which Topics a Node has is a Node parameter,
  and is edited like other Node parameters.
- A Named Node List (NNL) is an ordered list, and a Node can appear in it more
  than once (unless the list was created as `unique`), so it can be used for
  things like a call order. Being in a list is not a stable property of a Node;
  the list is a structure that Nodes are placed into, and the map of NNLs is kept
  at the Graph level, like the maps of Nodes, Edges and Topics. So NNLs are
  modified through `fzgraph` (or directly through the server's
  `/fz/graph/namedlists/...` API, which does not need shared memory).

Note: at present there is no command line option for creating a Topic on its own.
How Topics come into being is described in the manual, section I.4.

These arrangements are not hard to change: a different set of core programs could
be built from the same library code.

Source: Trello card CIkXDdyE (archived in ~/.formalizer/archive/trello/cards/).

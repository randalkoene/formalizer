# Coding conventions and design principles

These are conventions that Formalizer 2.x code follows. They are guidelines, not
rules enforced by tools.

## Small pieces, logically separated

- Break code into many logically separated files, and keep each file short enough
  to understand on its own.
- Keep functions small. Prefer more function calls over long functions; the
  compiler takes care of optimization.
- Keep code that is exposed through interfaces (library calls) in different files
  from code that is used only internally or only by one program:
  - Core library code lives in `core/include` (headers) and `core/lib` (sources).
    It is declared inside `namespace fz { ... }`.
  - Program code lives in the program's own directory (e.g. `core/fzedit`,
    `tools/interface/fzloghtml`). Programs use the library with
    `using namespace fz;` and do not add their own code to the `fz` namespace.
    The `boilerplate` library templates (`cpp_bp_lib*_template`) put new library
    code in `namespace fz`.

See [directory_layout.md](./directory_layout.md) for where things go.

Source: Trello card vfpmK0EC (archived in ~/.formalizer/archive/trello/cards/).

## Build interfaces for the web and the command line

There can be any number of interfaces to the Formalizer (in any language, on any
device), but the data is kept consistent in one database (which can be mirrored
and backed up). New tools are built around two interface types:

- **Web interfaces**, because web development is broad, well supported and
  advances quickly. In the Formalizer these are CGI scripts (`<program>-cgi.py`)
  and HTML pages served by Apache, plus the `/fz/...` paths of `fzserverpq`.
- **The command line**, because it has the fewest dependencies and works in the
  most difficult circumstances.

Other interface types (Android apps, desktop GUIs, ...) are secondary and can be
built around those two.

Terminal browsing with `w3m` is supported where it is cheap to do so: there are
`*-term.sh` scripts for some pages (e.g. in `tools/system/top`,
`tools/interface/fzloghtml` and `tools/interface/fzlogtime`). w3m depends on locally
running CGI and cannot use color or layout the way a full browser can, so web
pages are designed for a full browser first and kept usable from w3m where
possible.

In practice: when building a new tool, provide both a command line interface and
a web interface to it (typically a C++ or Python program plus a `-cgi.py`
wrapper).

Source: Trello card Be45bvpT (archived in ~/.formalizer/archive/trello/cards/).

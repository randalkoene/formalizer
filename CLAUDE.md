# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Formalizer is a personal productivity system framework — a layered set of tools, protocols, and data structures for managing goals, tasks, scheduling, and logging. It runs on a local Linux machine with a PostgreSQL backend, a C++ data server (`fzserverpq`), and a web interface served via Apache CGI.

## Architecture

The system is organized in three abstraction layers:

1. **Data Layer** (`core/include/`, `core/lib/`) — C++ types (`Graphtypes`, `Logtypes`) and Python wrappers. The graph is the central data structure: Nodes (tasks/goals) connected by dependency edges, and Log chunks (time-tracked work entries).
2. **Database Layer** (`core/lib/*postgres*.cpp`) — PostgreSQL access via `libpq`. Key files: `Graphpostgres.cpp`, `Logpostgres.cpp`.
3. **Data Server Layer** (`core/fzserverpq/`) — `fzserverpq` is a persistent TCP server that holds the graph in shared memory and serializes database access. Most other tools communicate with it via TCP calls rather than hitting PostgreSQL directly.

Above the core:
- **`core/`** — essential programs (`fzgraph`, `fzlog`, `fzquerypq`, `fzupdate`, `fzedit`, `fztask`, etc.)
- **`tools/interface/`** — CGI scripts and HTML interfaces (web UI served at `localhost` via Apache)
- **`tools/system/`** — higher-level system tools (scheduling, day/early wizard, metrics, replicate)
- **`tools/glue/`** — shell scripts for daily/hourly automation (`fzdaily.sh`, `fzhourly.sh`)
- **`tools/conversion/`** — migration tools from earlier Formalizer versions

Python tools communicate with `fzserverpq` via `core/include/tcpclient.py` and `Graphaccess.py`. C++ tools use `core/lib/tcpclient.cpp` and `apiclient.cpp`.

## Building

Build the core library first, then individual tools:

```bash
# Build core library
cd core/lib && make

# Build a specific C++ component
cd core/fzserverpq && make

# Build everything (library + all compilable components)
make build

# Use fzbuild shorthand (from tools/dev/fzbuild/)
fzbuild -C   # clean
fzbuild -M   # make
fzbuild -r   # clean + make
```

## Installing / deploying

```bash
# Refresh symlinks and deploy all executables, CGI scripts, web files
make executables   # requires sudo for CGI/web dirs

# Full deploy: executables + doxygen + tests
make
```

Executables land in `~/.formalizer/bin/`. CGI scripts go to `/usr/lib/cgi-bin/`. Web files go to `/var/www/html/` and `/var/www/html/formalizer/`.

## Configuration

Config files live in `~/.formalizer/config/<component>/` as a flat JSON-like subset:

```json
{
"variable_name" : "value",
"count" : 42
}
```

Initialize config directories with:

```bash
fzsetup.py -C
```

## Key conventions

- **C++ programs** follow a boilerplate pattern (see `tools/dev/boilerplate/`): a main `.cpp`, a `.hpp`, and a `Makefile`. Each program has a version header.
- **CGI variants** of programs are named `<program>-cgi.py` and call the compiled binary or act as the API endpoint.
- **Python shared utilities** in `core/include/` (`fzhtmlpage.py`, `fzcmdcalls.py`, `fzmodbase.py`, etc.) are symlinked into `/usr/lib/cgi-bin/` for CGI use.
- **Named Node Lists (NNLs)** are a key graph concept — sets of nodes identified by name, used heavily in scheduling and filtering.
- **`fzserverpq` must be running** for most tools to work. Start it with `fzserverpqd.sh`.

# Architecture: data servers and library access

This explains why the Graph is held by a memory-resident server while the Log is
read and written through core library calls, and why there is one authoritative
data server.

## The Graph: a memory-resident server

`fzserverpq` is expected to be running. It loads the Graph from the database and
keeps it resident in a shared-memory segment. Reasons:

- Graph access gets complex quickly because of the dependency hierarchy (e.g.
  inherited target dates, sorting by effective target date), and it is needed
  very often.
- A resident copy speeds this up a great deal, especially because programs on the
  same machine can read the Graph directly in shared memory
  (`Graph_access`, `graphmemman.find_Graph_in_shared_memory()`).

Changes go through the server: a client either puts a stack of
`Graph_modifications` in a shared-memory segment and sends the segment name to
the server's TCP port, or sends a `/fz/...` request on that port. The server
changes the in-memory Graph and then writes the change to the database. See
[tracing_actions.md](./tracing_actions.md) and the
[fzedit README](https://github.com/randalkoene/formalizer/blob/master/core/fzedit/README.md).

## The Log: library access

The Log is queried and changed directly by the programs that need it (e.g.
`fzlog`, `fzloghtml`, `fzlogdata`), through core library functions in
`Logpostgres` and `Logaccess`. The Log is large, and most of the time a program
needs either:

- a recent part of the Log, or a time interval (`load_partial_Log_pq()` with a
  `Log_filter`, `load_last_chunk_and_entry_pq()`), or
- the history of a Node, which can involve entries anywhere in the Log. Node
  histories are cached in a database table (`load_Node_history_cache_entry_pq()`;
  the cache is rebuilt by `fzquerypq -R histories`, which the daily script runs
  through `local_node_histories_refresh.py`).

With partial loading and cached histories this is fast enough, and PostgreSQL's
own caching helps. Converting from the stored format to `Log_chunk` and
`Log_entry` objects may be one of the main costs (not profiled).

Open questions from when this was decided (2020):

- Tools such as `fzloghtml` use the library to access the database directly, so
  arguably they are core components, though they live in `tools/interface`.
- Shared memory plus TCP works very well for the Graph, and could also speed up
  some Log access.

## One authoritative data server

There is a single authoritative data server for Formalizer data, `fzserverpq`,
tied to one database format (PostgreSQL). The alternative is several specialized
servers (for the Graph, the Log, Metrics). That could make sense if those kinds of
data are rarely needed together, or if only some of them (the Graph) should be
kept in memory.

Current state:

- `fzserverpq` serves the Graph (and Named Node Lists, Topics and Graph-related
  requests such as `/fz/graph/logtime`, which updates a Node's completion after
  time was logged).
- `core/fzserverpq-log` was started (2021) as a memory-resident Log server. It
  can load the Log and listen on a port, but it prints "This is a stub", its
  request handlers are not implemented, and it is commented out of
  `EXECUTABLES` in the root `Makefile`. Log access still goes through the library.
- There is no `fzserverpq-graph` or `fzserverpq-metrics`. Metrics are handled by
  `fzmetricspq`, which is a command line program, not a server.

Source: Trello cards 9azOXQn4 and is7NhBgQ (archived in ~/.formalizer/archive/trello/cards/).

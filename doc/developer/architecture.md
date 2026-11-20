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

## Inside fzserverpq: two threads

`fzserverpq` runs two threads (`USE_MULTI_THREADING`, defined in `fzserverpq.hpp`):

1. **The listen thread** (the main thread, `server_socket_listen()` in
   `core/lib/tcpserver.cpp`). It accepts a connection, reads the request string
   and decides what kind it is:
   - `PING` and `STOP` are answered right away on this thread. `STOP` (sent by
     `fzgraph -z`) ends the listen loop.
   - A request that starts with `GET `, `PATCH ` or `FZ ` (browser and port API
     requests) goes into the queue `special_FIFO`.
   - Anything else is taken to be the name of a shared-memory segment with a stack
     of `Graph_modifications` or a data request, and goes into the queue `shm_FIFO`.

   Putting a request in a queue only stores the open socket and the request string;
   the socket stays open until the request is handled.
2. **The queue thread** (`queue_handling_thread_func()` in `fzserverpq.cpp`). It
   takes one request from the queues, runs its handler, closes the socket, and
   goes on to the next request without a pause. `special_FIFO` is always emptied
   before `shm_FIFO`. When both queues are empty it waits on the condition
   variable `queueCV`, which is notified whenever a request is added, or at most
   250 ms (`idle_check_ms`), after which it checks the `listen` flag again.

Because of this, every request handler runs on the queue thread, one at a time.
The Graph in memory and the database are only changed from that thread, so
handlers do not need locks against each other. The two threads share only:

- the two queues, guarded by `queueMutex`, and `queueCV`;
- the `listen` flag (`std::atomic<bool>`), which ends both loops;
- the error queues and error trace (`ErrQ`, `WarnQ`, `errtracer`; see
  [error_handling.md](./error_handling.md)). Since 2026-11-20 each thread has its
  own `errtracer` (`thread_local`), and one lock in `error.cpp` guards all
  `Errors` queues. Before that, both threads changed the same trace and queues,
  which is a data race (Node 20261102153502.1).

Consequences worth knowing:

- Until 2026-11-20 the queue thread slept 50 ms after every request, so a burst
  of N requests took at least N × 50 ms.
- Stopping: `fzgraph -z` sends `STOP`, which the listen thread handles itself;
  the queue thread notices within 250 ms. `/fz/_stop` (the "Stop Server" button on
  the FZ Admin page) is handled on the queue thread, which sets `listen` to false
  and then sends `STOP` to the server's own port, because the listen thread is
  waiting in `accept()` and would otherwise not notice until the next connection
  arrived. Either way the program exits with status 0, so `fzserverpqd.sh` does
  not restart it.
- While the listen thread waits in `read()` for a client's request, no other
  connection is accepted. Browsers open spare connections that may send nothing,
  so since 2026-11-20 this first read waits at most 3 s (`SO_RCVTIMEO`,
  `first_read_timeout_s` in `tcpserver.cpp`) and then closes the connection.
  Before, one such connection held up all requests, including a Stop Server
  request, for over a minute. A listen thread that never waits on a single
  client (e.g. with `poll()`) would remove the 3 s as well.
- The kernel holds up to `SOMAXCONN` connections while the listen thread is busy.
  Until 2026-11-20 the backlog was 3, and in a burst the extra connections were
  dropped and retried by their clients only after 1 s.

Other programs on the same machine read the Graph directly in shared memory, and
they map the segment for writing (`bi::open_only` in `Graphtypes.cpp` and
`Graphmodify.cpp`, changed from `open_read_only` at some point). These programs
run as separate processes, so the two threads above do not coordinate with them.
For example, computing an effective target date (`Node::inherit_targetdate()`)
resets and sets the traversal `semaphore` of Nodes in the shared Graph. Two such
programs, or one such program and a Graph change in the server, can run at the
same time.

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

# fzserverpq - Memory-resident Graph server for the Formalizer environment

fzserverpq is the Postgres-compatible version of the C++ implementation of the Formalizer graph data server.

**Note that some examples of routes through fzserverpq are documented in the
fzgraph README.md.** The full list of port API routes is shown by `fzserverpq -h`.
Topics are created with `/fz/graph/topics/add?tag=<tag>[&title=<title>]` (see the
fzgraph README, "Making a Topic").

fzserverpq runs two threads: a listen thread that accepts requests and puts them in
a queue, and a queue thread that runs every request handler, one at a time. See
[doc/developer/architecture.md, "Inside fzserverpq: two threads"](../../doc/developer/architecture.md#inside-fzserverpq-two-threads).

Stop the server with `fzgraph -z` (it sends `STOP`), and start it with `fzserverpqd.sh`.

For more information see:

- https://docs.google.com/document/d/1rYPFgzFgjkF1xGx3uABiXiaDR5sfmOzqYQRqSntcyyY/edit#heading=h.tarhfe395l5v
- https://trello.com/c/S7SZUyeU

Randal A. Koene, 2020

# Error handling (ErrQ and WarnQ)

C++ Formalizer programs report non-fatal problems through two global queues,
declared in `core/include/error.hpp` and implemented in `core/lib/error.cpp`:

- `fz::ErrQ` for errors,
- `fz::WarnQ` for warnings.

Both are instances of the class `Errors`. A queue collects entries and writes them
to a destination file (or standard output).

## Adding entries

Use the macros and functions in `error.hpp`:

- `ADDERROR(f, e)` and `ADDWARNING(f, e)` push an entry. `f` is an identifying
  label, normally the function name (`__func__`), and `e` is the message.
- `ERRRETURNFALSE(f, e)` and `ERRRETURNNULL(f, e)` push an error and return.
- `standard_error(msg, __func__)` pushes an error, prints it with `VERBOSEERR`,
  and returns `false`. `standard_warning(msg, __func__)` pushes a warning and
  prints it with `VERYVERBOSEOUT`.

Each entry is written as:

```
[<trace>] <function>: <message>
```

## The trace prefix

The `<trace>` part depends on compile-time definitions:

- By default, a cooperative stack trace is kept in the global `errtracer`.
  Functions that call `ERRTRACE` (or `ERRENTER(label)`) at the top add their name
  to the trace for as long as they run. `ERRHERE(".milestone")` and
  `ERRHINT(...)` update the current level to show where in the function the
  program was. The trace is printed as `func1:func2:func3.milestone`.
- With `NO_ERR_TRACE` defined, only a single global hint string (`errhint`) is
  kept, set by the same macros.
- With both `NO_ERR_TRACE` and `NO_ERR_HINTS` defined, the macros do nothing.

A queue can instead put a time stamp in that position: `enable_timestamping()`
(the default is `enable_tracing()`). `fzserverpq` uses this for its request log
`ReqQ`, which is also an `Errors` object.

## Cached or immediate output

- By default a queue is cached: entries are kept in memory and written when the
  program exits. `formalizer_standard_program::init()` registers exit hooks that
  write both queues (`clean_exit_wrapper()`) and print a summary of how many
  errors and warnings were logged and where (`error_summary_wrapper()`, not shown
  with `-q`).
- After `disable_caching()`, each new entry is written immediately. This is
  useful for long-running servers and when hunting crashes, because cached
  entries are lost if the program crashes. (`dil2graph` does this.)
- When output is written and it is the first output of the queue, a line
  `Time Code at START OF PROGRAM: ...` is written first. If more than a day has
  passed since the last time code, a `Time Code UPDATE: ...` line is added. This
  helps with servers that run for a long time.
- A cached queue is written once at exit and overwrites the destination file.
  With caching disabled, the first write overwrites the file and later writes in
  the same run append. Whether that first write truncates or appends is fixed at
  compile time by `LOG_TRUNC_MODE` and `ERRWARN_LOG_MODE` in `error.hpp`
  (truncate at present).
- When the destination is `STDOUT`, `Errors::output()` prints the queue but does
  not clear it. With caching disabled, earlier entries are therefore printed again
  with each new entry.

## Destinations

- Defaults (compiled in): `/tmp/formalizer.core.error.ErrQ.log` and
  `/tmp/formalizer.core.error.WarnQ.log`.
- Configuration file `~/.formalizer/config/error/config.json`, read in
  `formalizer_standard_program::init()` through `ErrQ.init()`. Recognized
  parameters: `errlogpath`, `warnlogpath`, `errcaching` and `warncaching`
  (`true` or `false`, for `ErrQ` and `WarnQ` respectively). Before 2026-11-02,
  `warncaching` was not read, because the line meant for it repeated
  `errcaching`, and every flag set through `CONFIG_TEST_AND_SET_FLAG` was also
  logged as "not recognized" because that macro did not return.
- Command line: every standard program accepts `-E <errfile>` and
  `-W <warnfile>`, which override the configuration.
- The destination `STDOUT` sends the queue to standard output. This is the most
  practical choice when running a program as a CGI script or testing it from a
  web page, because the web server user (`www-data`) often cannot write to the
  configured log file locations. CGI scripts typically call programs with
  `-E STDOUT`.

`fzserverpq` is started by `fzserverpqd.sh` with
`-E /var/www/html/formalizer/formalizer.core.server.error.ErrQ.log`, and its
current queue can be viewed at `/fz/ErrQ` on the server port.

## Not implemented

These ideas from the original design are not implemented:

- Catching crashes (e.g. segfaults) to write the queues before exiting. There is
  an experimental handler in `error.cpp` under `INCLUDE_EXPERIMENTAL`, which is
  not enabled.
- Caching up to N entries before writing.
- Choosing append or truncate at run time.

## Python

Python tools use `core/include/error.py`, which defines the same exit codes as the
C++ `exit_status_code` enum (keep the two in sync) and helpers such as
`exit_error()`. It does not implement ErrQ/WarnQ queues.

Source: Trello card 8lu3hhrF (archived in ~/.formalizer/archive/trello/cards/).

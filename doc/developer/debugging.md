# Debugging hints

Practical hints for finding problems in Formalizer code. See also
[error_handling.md](./error_handling.md) for where errors and warnings are written.

## A program reports a database error

Run the same command again with `-Q log`. Every program that uses
`Postgres_access` (C++ programs with database access) accepts:

```
-Q normal   run database commands (default)
-Q log      run database commands and also log them
-Q sim      only log database commands, do not run them
```

`-Q log` is usually better than `-Q sim`, because later database calls often
depend on data returned by earlier ones (e.g. Log chunk data used to find specific
Log entries), and with `-Q sim` those earlier calls return nothing.

The log is written when the program exits, to the file set by `simPQfile` in
`~/.formalizer/config/fzpostgres/config.json` (the default file in the repository
sets `/var/www/html/formalizer/formalizer.core.fzpostgres.SimPQ.log`). `-h` shows
the current path. Note that the exit hook that writes this file is registered while
the `fzpostgres` configuration file is processed, so the file is only written if
that configuration file exists and sets at least one parameter.

For the running server, the same modes can be switched with
`/fz/db/mode?set=<run|log|sim>` on the server port (also on the FZ Admin page),
and the logged calls are shown at `/fz/db/log`.

This is an efficient way to find a malformed PostgreSQL command. To find where it
was built, start at the top-level calling function and follow the calls until you
reach the function that creates the command string (most are in
`core/lib/Graphpostgres.cpp`, `Logpostgres.cpp` and `fzpostgres.cpp`).

## Segmentation faults

- Usually the cause is a pointer: a `nullptr`, or an uninitialized pointer.
- Sometimes it is an index out of range, e.g. in a plain C array.
- Without a debugger or special build: put flushing print statements in the code
  to narrow down where the crash happens. Add `#define USE_COMPILEDPING` at the
  top of the source file (before including `error.hpp`, or pass
  `-DUSE_COMPILEDPING` in the Makefile) and place lines such as
  `COMPILEDPING(std::cout, "ping-1\n");`. These print and flush immediately, and
  also work in constructors of global objects, before `main()` begins. Then check
  the code between the last ping printed and the next one for pointers that could
  be `nullptr`.
- `ErrQ.enable_pinging()` together with `ADDERRPING("label")` puts ping entries in
  the error queue instead; combine it with `ErrQ.disable_caching()` so that the
  entries are written before the crash.
- Do not call configuration loading or other library code from constructors of
  global objects. The order in which globals in different source files are
  initialized is undefined, and this causes crashes that are hard to trace (see
  the comment on `configbase::load()` in `core/include/config.hpp`).

## Hard-to-read C++ compile and link errors

**`cannot bind non-const lvalue reference of type ... to an rvalue`**: C++ does not
allow a non-const reference to bind to a temporary. For example:

```
int a;
double &b = a;   // the int is converted to a temporary double
```

**A reference error on a line with `emplace()`**, such as "binding reference of
type `T&` to `const T` discards qualifiers": related to the previous one.
`emplace()` takes references, and a reference to a short-lived temporary (e.g. a
function's return value) can cause this. Background:
https://stackoverflow.com/questions/1565600/how-come-a-non-const-reference-cannot-bind-to-a-temporary-object

**`static assertion failed: std::vector must have a non-const, non-volatile value_type`**:
standard containers of `const T` are not allowed. Use a container of `T`, or one of
the alternatives in
https://stackoverflow.com/questions/6954906/does-c11-allow-vectorconst-t

**`multiple definition of ...`**: one common cause is a function defined (not only
declared) in a header file without `inline`.

**A wall of text about a container type, with `use of deleted function` near the
end**: an implicitly defined special member function (e.g. the copy constructor)
is not available. Somewhere in the text there is usually a line saying it "is
implicitly deleted because the default definition would be ill-formed". Find which
function is needed and define it explicitly.

**`passing 'const ...' as 'this' argument discards qualifiers` in an implicit
`operator=` during a copy constructor**: first find exactly which class and which
constructor the error is about. Then look for a const value used where a non-const
one is needed:

- A function parameter written as `(const T v)` that should be `(const T & v)`.
- Member variables declared `const` that are then assigned in a constructor or
  member function. Instead, leave the members non-const and declare the object
  `const` where needed, or make the members `private`/`protected` and control
  access.

**General ways to corner a wall-of-text error:**

1. Comment out the newly added code in blocks and recompile until the error
   disappears. Then narrow it down to the line.
2. Check for `auto` with a function that returns a reference, e.g.
   `auto chunks = log.get_Chunks();`, which makes a copy. Write the reference type
   explicitly, e.g. `Log_chunks_Map & chunks = log.get_Chunks();` (or `auto &`).

## Python: `KeyError` on a configuration dictionary

If the `dict` is a loaded configuration (such as `config`), the configuration may
not have been loaded in this context. This happens when a function in one Python
script is called from another script (e.g. a function of `logentry` called from
`fztask`), and the calling script did not load and add that configuration. Load the
configuration in the caller, and use a default value for missing parameters (e.g.
`config.get('name', default)`).

Source: Trello card XGqeLa9k (archived in ~/.formalizer/archive/trello/cards/).

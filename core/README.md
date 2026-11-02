# core - Formalizer core components

This directory of the Formalizer source code hierarchy is for header files, library objects, programs, scripts, templates and other components that are designated as `core` components of the Formalizer environment.

The criteria for determining if something belongs in `core` were developed on the Trello card at https://trello.com/c/Vz4ujwgb.

In brief, as of 2020-09-09, these criteria are that something belongs in `core` if:

1. It is an essential component. There is no other lower-level program that supplies the essential content.

2. A program performs a task that is clearly categorized as belonging to one of the first three abstraction layers: the **Data Layer**, the **Database Layer**, or the **Data Server Layer**.

### Architecture: abstraction layers

The Formalizer is built to be modular and granular, with abstraction layers. The first three layers are the `core`:

1. **Data Layer:** the data structures and their methods, e.g. `Graphtypes` and `Logtypes` in `core/include` and `core/lib`.
2. **Database Layer:** storing and retrieving those structures in PostgreSQL through `libpq`, e.g. `Graphpostgres` and `Logpostgres`.
3. **Data Server Layer:** `fzserverpq`, which keeps the Graph resident in shared memory and carries out changes.

Core programs and tools are built on top of these layers and use them through their library interfaces. The full description is in the [Abstraction Layers](https://docs.google.com/document/d/1rYPFgzFgjkF1xGx3uABiXiaDR5sfmOzqYQRqSntcyyY/edit#heading=h.mjghfxzra8k) subsection of the Google Doc "The System and the Formalizer". For why the Graph has a server and the Log is accessed through the library, see [doc/developer/architecture.md](../doc/developer/architecture.md).

Source: Trello card Vz4ujwgb (archived in ~/.formalizer/archive/trello/cards/).

### Dependencies

- The essential `core` *Data Layer* components `Graphtypes.hpp` and `Logtypes.hpp` depend on the `utfcpp` library,
an open source library for UTF8 text safety.
- The *Database Layer* components `Graphpostgre.hpp` and `Logpostgres.hpp` depend on the authoritative Postgres C library `libpq`.

### About configuration files

Core libraries and Formalizer standardized programs expect configuration files arranged within a `.formalizer/config/this-component/` directory, where `this-component` is the library component or program in question. The configuration directories can be initialized by running `fzsetup.py -C`.

The format of the configuration files is is a subset of JSON, as follows:

```

    {
    "variable_name1" : "some string value",
    "variable_name2" : 157,
    "variable_name3" : "SOME_IDENTIFIER_OR_LABELED_SETTING"
    }

```

Reasons:

- It may pay off to remain close to a known standard when then is little performance benefit to be had by using one of the others.
- That format appears to be immediately loadable in Python as a `dict`.
- Colon is just as good as equal sign.
- Comma is just as good as semicolon.
- By avoiding the recursive depth of regular JSON for now, parsing is kept very simple.

For more, see https://trello.com/c/4B7x2kif.

### Early initialization: `init()` registration

A Formalizer standard program (a class derived from `formalizer_standard_program`, see `core/include/standard.hpp`) calls `formalizer_standard_program::init(argc, argv, ...)` at the start of `main()` (usually through its own `init_top()`). Some objects need their own initialization to run at that point: after `main()` has been entered, but before command line options are parsed. They cannot do this in their constructors, because many of them are global objects or members of the global program object, and the order in which globals in different source files are constructed is undefined.

The `main_init_register` class provides this:

1. A class that needs early initialization inherits `main_init_register` and defines the pure virtual `bool init()`.
2. Its constructor takes a reference to the `formalizer_standard_program`. The `main_init_register` constructor calls `fsp.add_registered_init(this)`, which appends the object to the program's `init_register_stack`.
3. In `formalizer_standard_program::init()`, after the exit hooks are set up and `ErrQ.init()` has loaded the error configuration, `init()` is called for every registered object, in the order of registration. Only then is the command line parsed.

The main use is configuration. The `configurable` class (`core/include/config.hpp`) inherits both `configbase` and `main_init_register`, and its `init()` calls `load()` to parse the component's configuration file. A program declares a class derived from `configurable` (e.g. `fze_configurable` in `fzedit`) and makes it a member of its program class, constructed with `config(*this)`. The configuration file is then guaranteed to be read before the command line is parsed, so command line options can override configured values.

Library components whose configuration parameters are global and always present can use the simpler `configbase` directly and call `load()` themselves. For example, the error queues use `err_configbase`, loaded by `ErrQ.init()` (see [doc/developer/error_handling.md](../doc/developer/error_handling.md)).

The mechanism is not specific to configuration: any object that must be initialized on entering `main()` in a standard program can inherit `main_init_register`. At present `configurable` is the only class in the source tree that does. `boilerplate` asks whether a new program's configuration should use the init register (`configurable`) or `configbase`.

Source: Trello card eyckMCRs (archived in ~/.formalizer/archive/trello/cards/).

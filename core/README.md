# core - Formalizer core components

This directory of the Formalizer source code hierarchy is for header files, library objects, programs, scripts, templates and other components that are designated as `core` components of the Formalizer environment.

The criteria for determining if something belongs in `core` were developed on the Trello card at https://trello.com/c/Vz4ujwgb.

In brief, as of 2020-09-09, these criteria are that something belongs in `core` if:

1. It is an essential component. There is no other lower-level program that supplies the essential content.

2. A program performs a task that is clearly categorized as belonging to one of the first three abstraction layers: the **Data Layer**, the **Database Layer**, or the **Data Server Layer**.

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

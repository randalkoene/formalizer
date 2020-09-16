# core - Formalizer core components

This directory of the Formalizer source code hierarchy is for header files, library objects, programs, scripts, templates and other components that are designated as `core` components of the Formalizer environment.

The criteria for determining if something belongs in `core` were developed on the Trello card at https://trello.com/c/Vz4ujwgb.

In brief, as of 2020-09-09, these criteria are that something belongs in `core` if:

1. It is an essential component. There is no other lower-level program that supplies the essential content.

2. A program performs a task that is clearly categorized as belonging to one of the first three abstraction layers: the **Data Layer**, the **Database Layer**, or the **Data Server Layer**.

### Notes

- The essential `core` *Data Layer* components `Graphtypes.hpp` and `Logtypes.hpp` depend on the `utfcpp` library,
an open source library for UTF8 text safety.
- The *Database Layer* components `Graphpostgre.hpp` and `Logpostgres.hpp` depend on the authoritative Postgres C library `libpq`.

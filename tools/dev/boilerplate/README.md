# boilerplate - Quickly prepare standardized Formalizer source code

The `boilerplate` tool facilitates Formalizer development work by simplifying the rapid generation of standardized source code stubs, while adhering to up-to-date Formalizer standards. Using `boilerplate` helps to ensure those standards are applied.

The same `boilerplate` tool is used to generate stubs for multiple languages by specifying `c++`, `python`, etc.

## Formalizer standardized programs

A Formalizer standardized program is built with awareness of the Formalizer environment and its standards:

- Its source code is in the Formalizer source tree, in the standard place (`core/<name>`, `core/lib` + `core/include` for core library code, or `tools/[<category>/]<name>`; see `doc/developer/directory_layout.md`).
- It uses the `core` library, and may also use headers and objects of other programs in the tree.
- It has a module ID that describes its place in the environment (`FORMALIZER_MODULE_ID`, e.g. `"Formalizer:Server:Log:Postgres"`) and a version (in `version.hpp`).
- A C++ standardized program declares a class that inherits `formalizer_standard_program` (`core/include/standard.hpp`) and a global object of that class. This gives it the standard command line options (`-E`, `-W`, `-q`, `-V`, `-v`, `-h`), standard error and warning reporting through `ErrQ` and `WarnQ` (see `doc/developer/error_handling.md`), and the standard exit handling.
- It uses standard configuration files in `~/.formalizer/config/<name>/config.json`, which `fzsetup.py -1 config` prepares for programs listed in `core/include/executables.py` or `core/include/coreconfigurable.py`. See "Early initialization: `init()` registration" in `core/README.md`.
- It is known to the root `Makefile` (e.g. in `EXECUTABLES`), and `fzinfo.py -E` lists the executables provided.
- In many cases it can be built through `fzbuild.py`.

## Making a standardized program with boilerplate

Run `boilerplate -T cpp` (C++) or `boilerplate -T python` (Python). It asks:

- the component name;
- whether it is a `core` or a `tools` component;
- for `core`: whether it adds library objects (to `core/lib` and `core/include`) or is a standalone program (in `core/<name>`);
- for `tools`: an optional category (e.g. `system/metrics`), giving `tools/<category>/<name>`;
- whether it uses a standardized config file, and if so (C++) whether it is configured at program start through the init register (`configurable`) or uses `configbase`;
- (C++ programs) whether to include template rendering files (`render.cpp`, `render.hpp` and a `templates` directory, for use with `templater`);
- for programs: an abbreviation (used in class names, e.g. `fze` for `fzedit`), the module ID, and the version.

For a C++ program it creates `<name>.cpp`, `<name>.hpp`, `version.hpp`, `Makefile`, `README.md` and an `obj` directory (plus the render files if requested). For a Python program it creates `<name>.py` and `README.md`. It refuses to overwrite existing files.

`boilerplate` does not register the new program anywhere. At the end it prints reminders to:

- add the program to `EXECUTABLES` in the root `Makefile`, and check `SYMBIN`, `CGIEXE` and `WEBINTERFACES`;
- add it to `core/include/executables.py` or `core/include/coreconfigurable.py` if it uses a config file;
- check whether `fzinfo` needs information about it;
- consider adding a build rule to `fzbuild.py`.

The generated C++ code is meant to compile as is (unverified for the current templates): run `make` in the new directory. Then fill in the stubs (`options_hook()`, `usage_hook()`, `set_parameter()`, the flow control in `main()`) with the program's own features.

Small Python scripts, in particular small CGI scripts, are often written without `boilerplate` (see section IV.6 of the manual).

Source: Trello card AqLNBRWx (archived in ~/.formalizer/archive/trello/cards/).

Randal A. Koene, 2020

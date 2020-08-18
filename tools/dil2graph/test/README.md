## Formalizer:Tools:dil2graph: Unit Tests

Unit tests for (class member) functions of the dil2graph tool.

Please note that these are *Unit Tests*. They are intended to test fundamental behavior required of functions.
This is distinct from the validation tests that the dil2graph tool carries out when performing its task to convert Formalizer data structures from DIL Hierarchy to Graph format. Those validation tests are included directly in the `dil2graph.cpp` source code.

### Compiling and running unit tests

Do one of these:

1. From the parent directory (`../`), run: `make test`

2. From this directory, run: `make`

Then, run all tests:

```
./test
```

Or, run a specific test:

```
./test <specific-test-name>
```

Or, see the tests that are available:

```
./test -h
```

### Note

*The Unit test have not yet been implemented, but they will probably use Catch2.*

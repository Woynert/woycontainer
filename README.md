## Run coverage for fuzz test.

```
make mesonSetupDebugClang
make compile
./build/wmap_test_fuzzy -runs=1000
make coverage
make coverageClean # <- To start from fresh state.
```

## Tests

|           | Usable | Manual test   | Auto test   | Fuzz test   | Tests include naive version | Included tests anti internal memory leaks? |
| --------- | ---    | ------------- | ----------- | ----------- | -------                     | ---                                        |
| Array     | X      |               |             |             |                             |                                            |
| Vector    | X      |               |             |             |                             |                                            |
| StrVector |        |               |             |             |                             |                                            |
| Strpool   | X      | X             | X           | X           | X                           |                                            |
| Slot      | X      | X             |             |             |                             |                                            |
| Wmap      | X      | X             | X           | X           | X                           |                                            |
| Wstrmap   | X      | X             | X           | X           | X                           |                                            |

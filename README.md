## Run coverage for fuzz test.

```
make mesonSetupDebugClang
make compile
./build/wmap_test_fuzzy -runs=1000
make coverage
make coverageClean # <- To start from fresh state.
```

## Tests

| Container                 | File      | Usable | Manual test | Auto test     | Fuzz test   | Tests include naive version | Included tests anti internal memory leaks? |
| ---------                 | ---       | ---    | ---         | ------------- | ----------- | -----------                 | -------                                    |
| Array                     | array.h   | X      |             |               |             |                             |                                            |
| Vector                    | da.h      | X      |             |               |             |                             |                                            |
| StrVector                 | N/A       |        |             |               |             |                             |                                            |
| Strpool                   | strpool.h | X      | X           | X             | X           | X                           |                                            |
| Slot                      | slot.h    | X      | X           |               |             |                             |                                            |
| Ordered hash map          | wmap.h    | X      | X           | X             | X           | X                           |                                            |
| Ordered string hash map   | wmapstr.h | X      | X           | X             | X           | X                           |                                            |
| Unordered hash map        | umap.h    | X      | X           |               |             |                             |                                            |
| Unordered string hash map | umapstr.h | X      | X           |               |             |                             |                                            |

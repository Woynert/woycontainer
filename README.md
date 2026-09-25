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
| String pool/allocator     | strpool.h | X      | X           | X             | X           | X                           |                                            |
| Slot                      | slot.h    | X      | X           |               |             |                             |                                            |
| Ordered hash map          | wmap.h    | X      | X           | X             | X           | X                           |                                            |
| Ordered string hash map   | wmapstr.h | X      | X           | X             | X           | X                           |                                            |
| Unordered hash map        | umap.h    | X      | X           |               |             |                             |                                            |
| Unordered string hash map | umapstr.h | X      | X           |               |             |                             |                                            |

## Others / Misc / Uncategorized.
* wring.h
* ringbuffer.h
* pool.h
* tree_simple.h
* wstrview.h
* woytest.h
* stringpool_quick.h (should I deprecated this?)
* sizedbuffer.h (Superseded by array.h?)
* list_simple.h

## Run tests and coverage (gcc)

```sh
cd test
make test
make coverage
```

## Run fuzzy tests and coverage (clang)

```sh
cd test
make fuzzteest
make coverageClang
```

## Tests

| Container                 | File        | Usable | Manual test | Auto test     | Fuzz test   | Tests include naive version | Included tests anti internal memory leaks? |
| ---------                 | ---         | ---    | ---         | ------------- | ----------- | -----------                 | -------                                    |
| Array                     | array.h     | X      |             |               |             |                             |                                            |
| Vector                    | da.h        | X      |             |               |             |                             |                                            |
| StrVector                 | N/A         |        |             |               |             |                             |                                            |
| String pool/allocator     | strpool.h   | X      | X           | X             | X           | X                           |                                            |
| Slot                      | slot.h      | X      | X           |               |             |                             |                                            |
| Ordered hash map          | wmap.h      | X      | X           | X             | X           | X                           |                                            |
| Ordered string hash map   | wmapstr.h   | X      | X           | X             | X           | X                           |                                            |
| Unordered hash map        | umap.h      | X      | X           |               |             |                             |                                            |
| Unordered string hash map | umapstr.h   | X      | X           |               |             |                             |                                            |
| Slotpool                  | slotpool.h  | X      | X           |               | X           | X                           |                                            |
| MapStrObj                 | mapstrobj.h | X      | X           |               |             |                             |                                            |

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

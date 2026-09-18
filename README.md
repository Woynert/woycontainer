# Run coverage for fuzz test.

make mesonSetupDebugClang
make compile
./build/wmap_test_fuzzy --runs=100
make coverage
make coverageClean # <- To start from fresh state.

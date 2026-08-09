# enajDB

If you changed CMake:

- Delete any `build/` folders
- `cmake -S . -B build`
- For address sanitization: `cmake -S . -B build-asan -DENABLE_ASAN=ON`

Recompilation:

- `cmake --build build`
- `cmake --build build-asan`

Testing:

- Run a test with `ctest -R ${TestSuiteName}$`
- See all tests with `ctest -N`

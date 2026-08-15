# enajDB

If you changed CMake:

- Delete any `build/` folders
- `cmake -S . -B build`
- For address sanitization: `cmake -S . -B build-asan -DENABLE_ASAN=ON`

Recompilation:

- `cmake --build build`
- `cmake --build build-asan`

Testing: Ideally use `build-asan` to catch memory errors

- See all tests:
  `ctest --test-dir build-asan -N`
- Run tests:
  `ctest --test-dir build-asan --output-on-failure`
- Run a specific test:
  `ctest --test-dir build-asan -R '${TestSuiteName}$' --output-on-failure`

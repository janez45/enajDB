cmake_minimum_required(VERSION 3.25)

project(enajDB)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(
    -Wall
    -g
)

add_executable(
    enajDB
    src/main.cc
)

# Fetch GoogleTest framework
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)

# Enable CTest infrastructure
enable_testing()

add_executable(
    tests 
    tests/tests.cc
)

target_link_libraries(tests
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(tests)
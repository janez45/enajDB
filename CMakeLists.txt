cmake_minimum_required(VERSION 3.25)

project(enajDB)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


add_compile_options(
    -Wall
    -g
)

# ----------------
# EnajDB library
# ----------------

add_library(enajDB_lib
    src/pager.cc
    src/table.cc
    src/utils.cc
)

# Headers exposed by enajDB_lib.
target_include_directories(
    enajDB_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ----------------
# EnajDB executable
# ----------------
add_executable(
    enajDB
    src/main.cc
)

# link the enajDB_lib, but if any executable links against enajDB, 
# they do not inherit enajDB_lib
target_link_libraries(enajDB PRIVATE enajDB_lib)

# Fetch GoogleTest framework
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)

# Enable ctest, which organizes/discovers/runs tests from CMake
enable_testing()

# Let tests/CMakeLists.txt define the test executables.
add_subdirectory(tests)

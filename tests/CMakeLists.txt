# Take the GoogleTest tests contained inside the tests executable and tell CTest about them.
include(GoogleTest)

# Pager unit tests
add_executable(
    pager_test
    pager/pager_test.cc
)

target_link_libraries(
    pager_test
    PRIVATE
        enajDB_lib
        GTest::gtest_main
)

gtest_discover_tests(pager_test)


# Table unit tests
add_executable(
    table_test
    table/table_test.cc
)

target_link_libraries(
    table_test
    PRIVATE
        enajDB_lib
        GTest::gtest_main
)

gtest_discover_tests(table_test)

# B+ tree unit tests
add_executable(
    b_plus_tree_test
    b_plus_tree/b_plus_tree_test.cc
)

target_link_libraries(
    b_plus_tree_test
    PRIVATE
        enajDB_lib
        GTest::gtest_main
)

gtest_discover_tests(b_plus_tree_test)


# End-to-end tests
add_executable(
    e2e_test
    e2e/cli_test.cc
)

# this tests the whole executable as a black box, so it does not depend on the libraries
target_link_libraries(
    e2e_test
    PRIVATE
        GTest::gtest_main
)

target_compile_definitions(
    e2e_test
    PRIVATE
        ENAJDB_EXECUTABLE="$<TARGET_FILE:enajDB>"
)


gtest_discover_tests(e2e_test)

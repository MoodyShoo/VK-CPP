cmake_minimum_required(VERSION 3.11)

project(kvstorage)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

enable_testing()

add_subdirectory(extern/googletest)

add_executable(kvstorage_tests 
    tests/kvstorage_tests.cpp)

target_link_libraries(kvstorage_tests PRIVATE gtest_main)

add_executable(kvstorage_perf_test
    tests/kvstorage_perf_test.cpp
)

target_link_libraries(kvstorage_perf_test PRIVATE gtest_main)

add_test(NAME KVStorageTests COMMAND kvstorage_tests)
set_tests_properties(KVStorageTests PROPERTIES LABELS "unit")

add_test(NAME KVStoragePerfTest COMMAND kvstorage_perf_test)
set_tests_properties(KVStoragePerfTest PROPERTIES LABELS "performance")
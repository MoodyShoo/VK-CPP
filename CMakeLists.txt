cmake_minimum_required(VERSION 3.11)

project(kvstorage)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

enable_testing()

add_subdirectory(extern/googletest)

add_executable(kvstorage_tests tests/kvstorage_tests.cpp)

target_link_libraries(kvstorage_tests PRIVATE gtest_main)

add_test(NAME KVStorageTests COMMAND kvstorage_tests)

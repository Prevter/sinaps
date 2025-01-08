cmake_minimum_required(VERSION 3.21)
project(sinaps)

set(CMAKE_CXX_STANDARD 20)

add_library(sinaps INTERFACE)
target_include_directories(sinaps INTERFACE include)

if (SINAPS_BUILD_TESTS)
    add_executable(sinaps_test "test/main.cpp")
    target_link_libraries(sinaps_test sinaps)
endif()
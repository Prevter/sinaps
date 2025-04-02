cmake_minimum_required(VERSION 3.21)
project(sinaps)

set(CMAKE_CXX_STANDARD 20)

add_library(sinaps INTERFACE)
target_include_directories(sinaps INTERFACE include)

if (SINAPS_GEN_SINGLE_HEADER)
    # read all files
    file(READ "include/sinaps/utils.hpp" SINAPS_UTILS_HPP)
    file(READ "include/sinaps/token.hpp" SINAPS_TOKEN_HPP)
    file(READ "include/sinaps/masks.hpp" SINAPS_MASKS_HPP)
    file(READ "include/sinaps/pattern.hpp" SINAPS_PATTERN_HPP)
    file(READ "include/sinaps.hpp" SINAPS_HPP)

    # remove '#include "' lines from all files
    string(REGEX REPLACE "#include \"[^\"]+\"\n" "" SINAPS_UTILS_HPP "${SINAPS_UTILS_HPP}")
    string(REGEX REPLACE "#include \"[^\"]+\"\n" "" SINAPS_TOKEN_HPP "${SINAPS_TOKEN_HPP}")
    string(REGEX REPLACE "#include \"[^\"]+\"\n" "" SINAPS_MASKS_HPP "${SINAPS_MASKS_HPP}")
    string(REGEX REPLACE "#include \"[^\"]+\"\n" "" SINAPS_PATTERN_HPP "${SINAPS_PATTERN_HPP}")
    string(REGEX REPLACE "#include \"[^\"]+\"\n" "" SINAPS_HPP "${SINAPS_HPP}")

    # remove '#pragma once' lines from all files
    string(REGEX REPLACE "#pragma once\n" "" SINAPS_UTILS_HPP "${SINAPS_UTILS_HPP}")
    string(REGEX REPLACE "#pragma once\n" "" SINAPS_TOKEN_HPP "${SINAPS_TOKEN_HPP}")
    string(REGEX REPLACE "#pragma once\n" "" SINAPS_MASKS_HPP "${SINAPS_MASKS_HPP}")
    string(REGEX REPLACE "#pragma once\n" "" SINAPS_PATTERN_HPP "${SINAPS_PATTERN_HPP}")
    string(REGEX REPLACE "#pragma once\n" "" SINAPS_HPP "${SINAPS_HPP}")

    # concatenate all files into a single header
    set(SINAPS_SINGLE_HEADER "#pragma once\n#ifndef SINAPS_SINGLE_HEADER\n#define SINAPS_SINGLE_HEADER\n\n${SINAPS_UTILS_HPP}\n${SINAPS_TOKEN_HPP}\n${SINAPS_MASKS_HPP}\n${SINAPS_PATTERN_HPP}\n${SINAPS_HPP}\n#endif // SINAPS_SINGLE_HEADER\n")
    file(WRITE "single_include/sinaps.hpp" "${SINAPS_SINGLE_HEADER}")
endif()

if (SINAPS_BUILD_TESTS)
    add_executable(sinaps_test "test/main.cpp")
    target_link_libraries(sinaps_test sinaps)
endif()
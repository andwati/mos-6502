cmake_minimum_required(VERSION 3.16)
project(mos6502 C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Output binaries in /build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add warnings and sanitizer options
include(cmake/compiler_warnings.cmake)

add_subdirectory(src)
add_subdirectory(tests)

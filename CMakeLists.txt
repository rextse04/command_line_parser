cmake_minimum_required(VERSION 3.30)
project(command_line_parser)

set(CMAKE_CXX_STANDARD 26)

include_directories(.)
add_subdirectory(./tests)

add_library(command_line_parser INTERFACE
        parser.hpp
        config_default.hpp)
target_include_directories(command_line_parser INTERFACE .)
cmake_minimum_required(VERSION 3.14)
project(rcrp C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "-Wall -Wextra -Wpedantic")

add_subdirectory(src)

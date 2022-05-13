cmake_minimum_required(VERSION 3.17)
project(malloc_tutorial C)

set(CMAKE_C_STANDARD 99)

add_executable(malloc_tutorial my_malloc.c my_malloc.h)
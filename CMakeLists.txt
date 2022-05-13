cmake_minimum_required(VERSION 3.17)
project(my_malloc C)

set(CMAKE_C_STANDARD 99)

<<<<<<< HEAD
aux_source_directory(. SOURCES)
add_executable(my_malloc ${SOURCES})
=======
add_executable(malloc_tutorial my_malloc.c my_malloc.h)
>>>>>>> 6582f4d8ef960cadc9ff78d9061b34e416821a17

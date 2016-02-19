cmake_minimum_required(VERSION 3.3)
project(lab2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")

set(SOURCE_FILES simsh1.c)
file(GLOB_RECURSE SOURCE_FILES "*.c")

include_directories("aux_files")

add_executable(simsh1 ${SOURCE_FILES})


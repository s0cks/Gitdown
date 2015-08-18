cmake_minimum_required(VERSION 3.2)
project(Gitdown)

include_directories(include)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

set(SOURCE_FILES main.c)
add_executable(Gitdown ${SOURCE_FILES} include/gitdown.h include/buffer.h include/document.h include/stack.h)
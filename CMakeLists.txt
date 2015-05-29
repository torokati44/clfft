cmake_minimum_required(VERSION 2.8)
project(clfft)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -pedantic")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

set(SOURCE_FILES
        src/main.cpp
        src/clwrapper.hpp
        src/clwrapper.cpp
        src/clbuffer.hpp
        src/clkernel.hpp)


add_executable(clfft ${SOURCE_FILES})
target_link_libraries (clfft OpenCL SDL2 GL GLU)

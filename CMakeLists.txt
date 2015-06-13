cmake_minimum_required(VERSION 2.8)
project(clfft)

include_directories("/usr/local/include/")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -pedantic")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

set(SOURCE_FILES
        src/main.cpp
        src/clwrapper.hpp
        src/clwrapper.cpp
        src/clbuffer.hpp
        src/clkernel.hpp
        src/bufferedrecorder.cpp
        src/bufferedrecorder.hpp)


add_executable(clfft ${SOURCE_FILES})
target_link_libraries (clfft OpenCL sfml-audio sfml-window sfml-system GL GLU)

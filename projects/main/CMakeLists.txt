cmake_minimum_required(VERSION 3.14)
project(dmwatcher VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LINK_LIBS Threads::Threads)

file(GLOB_RECURSE SRCS ../../src/main.cpp
)

file (GLOB_RECURSE HDRS ../../src/base/myassert.hpp
../../src/base/pch.hpp
)

include(../base.cmake)

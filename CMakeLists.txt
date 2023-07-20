cmake_minimum_required(VERSION 3.25)
project(cmd-args)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static")

add_executable(CmdArgs src/main.cpp src/cmd-args.hpp)
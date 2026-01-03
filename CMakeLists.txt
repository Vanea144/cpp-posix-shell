cmake_minimum_required(VERSION 3.13)

project(cpp-posix-shell)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)

add_executable(shell ${SOURCE_FILES})

target_include_directories(shell PRIVATE include)
target_compile_options(shell PRIVATE -Wall -Wextra -Wpedantic)


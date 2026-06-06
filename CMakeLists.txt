
cmake_minimum_required(VERSION 3.10)
project(Compilerfire)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Curses REQUIRED)

add_executable(compilerfire src/main.cpp)
target_include_directories(compilerfire PRIVATE ${CURSES_INCLUDE_DIRS})
target_link_libraries(compilerfire PRIVATE ${CURSES_LIBRARIES})
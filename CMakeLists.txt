cmake_minimum_required(VERSION 3.20)

project(AssemblyCompiler VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(external)
add_subdirectory(src)
add_subdirectory(tests)

cmake_minimum_required(VERSION 3.20)

project(AssemblyCompiler VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# ===== Options =====
option(BUILD_DOCS  "Build Doxygen documentation" OFF)
option(BUILD_TESTS "Build unit tests"            OFF)

add_subdirectory(src)

# if(BUILD_DOCS)
add_subdirectory(docs)
# endif()

if(BUILD_TESTS)
  include(CTest)
  enable_testing()

  add_subdirectory(external)
  add_subdirectory(tests)
endif()


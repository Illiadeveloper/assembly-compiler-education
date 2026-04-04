add_library(assembly_parser STATIC Parser.cpp)
add_library(assembly::parser ALIAS assembly_parser)
target_include_directories(assembly_parser PUBLIC ${PROJECT_SOURCE_DIR}/include)

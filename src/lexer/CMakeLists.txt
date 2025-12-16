add_library(assembly_lexer STATIC Lexer.cpp)
add_library(assembly::lexer ALIAS assembly_lexer)
target_include_directories(assembly_lexer PUBLIC ${PROJECT_SOURCE_DIR}/include)

add_library(assembly_opcodes STATIC OpcodeFactories.cpp)
add_library(assembly::opcodes ALIAS assembly_opcodes)
target_include_directories(assembly_opcodes PUBLIC ${PROJECT_SOURCE_DIR}/include)

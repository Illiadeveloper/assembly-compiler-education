add_library(assembly_encoder STATIC InstructionEncoder.cpp SizeResolver.cpp Layout.cpp ElfWriter.cpp)
target_link_libraries(assembly_encoder PUBLIC assembly::opcodes)
add_library(assembly::encoder ALIAS assembly_encoder)
target_include_directories(assembly_encoder PUBLIC ${PROJECT_SOURCE_DIR}/include)

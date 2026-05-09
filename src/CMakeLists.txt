add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(grouper)
add_subdirectory(encoder)
add_subdirectory(opcodes)

add_executable(${PROJECT_NAME} 
  main.cpp 
  Compiler.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
  assembly::lexer
  assembly::parser
  assembly::grouper
  assembly::opcodes
  assembly::encoder
)

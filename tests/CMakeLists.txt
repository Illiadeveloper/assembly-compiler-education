# Lexer Unit tests
add_executable(lexer_unit_tests unit/LexerUnitTests.cpp)
target_link_libraries(lexer_unit_tests PRIVATE gtest_main assembly::lexer)
target_include_directories(lexer_unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
add_test(NAME LexerUnit COMMAND lexer_unit_tests)

add_custom_target(run-unit-tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R LexerUnit
  DEPENDS lexer_unit_tests
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

enable_testing()

# Lexer Unit tests
add_executable(lexer_unit_tests unit/LexerUnitTests.cpp)
target_link_libraries(lexer_unit_tests PRIVATE gtest_main assembly::lexer)
target_include_directories(lexer_unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/inlcude)
add_test(NAME LexerUnit COMMAND lexer_unit_tests)


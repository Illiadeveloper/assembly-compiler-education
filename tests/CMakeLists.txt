# Lexer Unit tests
add_executable(lexer_unit_tests unit/LexerUnitTests.cpp)
target_link_libraries(lexer_unit_tests PRIVATE gtest_main assembly::lexer)
# target_include_directories(lexer_unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
add_test(NAME LexerUnit COMMAND lexer_unit_tests)

# Parser Unit tests
add_executable(parser_unit_tests unit/ParserUnitTests.cpp)
target_link_libraries(parser_unit_tests PRIVATE gtest_main assembly::parser)
# target_include_directories(parser_unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
add_test(NAME ParserUnit COMMAND parser_unit_tests)

include(GoogleTest)
gtest_discover_tests(lexer_unit_tests)
gtest_discover_tests(parser_unit_tests)

# add_custom_target(run-unit-tests
#   COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure 
#   DEPENDS lexer_unit_tests parser_unit_tests
#   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
# )

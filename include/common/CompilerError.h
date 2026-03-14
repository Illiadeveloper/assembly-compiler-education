/// @file CompilerError.h
/// @brief Definition of compiler error types and error reporting structures
#pragma once

#include "Token.h"

/// @brief Compilation stage when an error occured
enum class ErrorStage {
  LEXER,  ///< Error occurred during lexical analysis
  PARSER, ///< Error occurred during parsing
  CODEGEN ///<  Error occurred during code generation
};

/// @brief Represents a compiler error with location and message
struct CompilerError {
  ErrorStage stage;    ///< Compilation stage where the error happened
  SourceSpan span;     ///< Source location of the error
  std::string message; ///< Human-readable error description
};

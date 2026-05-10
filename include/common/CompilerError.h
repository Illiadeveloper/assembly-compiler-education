/// @file CompilerError.h
/// @brief Definition of compiler error types and error reporting structures
#pragma once

#include "Token.h"

/// @brief Compilation stage when an error occured
enum class ErrorStage {
  LEXER,          ///< Error occurred during lexical analysis
  PARSER,         ///< Error occurred during parsing
  GROUPER,        ///< Error occured during grouper
  ENCODER,        ///< Error occured during encoder
  SIZE_RESOLVER,  ///< Error occured during size solver
  ELF_WRITER      ///<  Error occurred during elf writer
};

/// @brief Represents a compiler error with location and message
struct CompilerError {
  ErrorStage stage;     ///< Compilation stage where the error happened
  SourceSpan span;      ///< Source location of the error
  std::string message;  ///< Human-readable error description
};

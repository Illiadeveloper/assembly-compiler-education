/// @file Token.h
/// @brief Token definitions used by the compilation pipeline.
///
/// Defines token types and token structure produced by the lexer
/// and consumed by subsequent compilation stages (parser, semantic analysis).
#pragma once
#include <string>

/// @brief Enumeration of all lexical token types.
enum class TokenType {
  WORD, /// Identifier or instruction mnemonic
  NUMBER, /// Numeric literal
  COMMA,
  LBRACKET,
  RBRACKET,
  COLON,
  PLUS,
  MINUS,
  STAR,
  INVALID /// Invalid or unrecognized token
};

/// @brief Represents a single lexical token.
///
/// A token consists of its type, textual value, and source location.
/// Tokens are produced by the lexer and consumed by the parser.
struct Token {
  /// Token category (identifier, number, symbol, etc.)
  TokenType type;
  
  /// Original textual representation of the token
  std::string value;

  /// Line number in source code where the token begins (1-based)
  int line;
};

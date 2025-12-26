/// @file Lexer.h
/// @brief Lexical analyzer for assembly source code.
///
/// Defines the Lexer class responsible for converting raw source text
/// into a stream of tokens as the first stage of the compilation pipeline.
#pragma once

#include <string>
#include <vector>

#include "common/Token.h"

/// @brief Represents a lexical error detected during tokenization.
///
/// Lexical errors do not stop the lexer; they are collected and
/// can be retrieved after tokenization is complete.
struct LexError {
  /// Line number where the error occurred (1-based)
  int line;

  /// Character position in the source string
  size_t pos;

  /// Human-readable error message
  std::string message;
};

/// @brief Assembly lexer converting source text into tokens
///
/// Performs lexical analysis as the first stage of the compilation pipeline.
/// The lexer is responsible for tokenization, comment handling, and
/// error reporting, but does not perform any syntactic or semantic checks.
class Lexer {
private:
  /// Full source code being tokenized
  std::string code;

  std::vector<LexError> errors;

  /// Current position in the source string
  size_t pos = 0;

  /// Current line number(1-based)
  int line = 1;

  /// @internal
  /// Returns true if lexer reached end of input
  bool eof() const;

  /// @internal
  /// Returns the current character without advancing the input position.
  /// Undefined behavior if called at end of input.
  char peek() const;

  /// @internal
  /// Returns the next character without advancing the input position.
  /// Undefined behavior if called at end of input.
  char peekNext() const;

  /// @internal
  /// Returns the current character and advances the input position.
  /// Invariant: after the call, pos points to the next unread character.
  char get();

  /// @internal
  /// Records a lexical error at the current source position.
  /// The error is stored internally and does not interrupt tokenization.
  void addError(const std::string &msg);

  /// @internal
  /// Skips whitespace characters and updates line counter.
  void skipWhitespace();

  /// @internal
  /// Skips a single-line comment starting at the current position.
  void skipComment();

  /// @internal
  /// Reads an identifier or instruction mnemonic.
  Token readWord();

  /// @internal
  /// Reads a numeric literal.
  Token readNumber();

public:
  /// Constructs a lexer for the given source code.
  ///
  /// @param source Full assembly source code to be tokenized.
  /// @note No tokenization is performed during construction.
  ///       Call tokenize() to start lexical analysis.
  explicit Lexer(const std::string &source);

  /// Tokenizes the input source code.
  ///
  /// @return Vector of tokens in source order.
  ///         Token positions correspond to original source locations.
  /// @note The lexer attempts to continue tokenization after errors.
  std::vector<Token> tokenize();

  /// Returns all lexical errors encountered during tokenization.
  std::vector<LexError> getErrors() const;
};

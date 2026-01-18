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
  /// Source location where the error occurred
  SourceSpan span;

  /// Human-readable error message
  std::string message;
};

/// @brief Assembly lexer converting source text into tokens.
///
/// Performs lexical analysis as the first stage of the compilation pipeline.
/// The lexer is responsible for:
/// - splitting source text into lexical tokens
/// - tracking source locations
/// - skipping whitespace and comments
/// - collecting lexical errors
///
/// The lexer does not perform any syntactic or semantic validation.
class Lexer {
private:
  /// Full source code being tokenized
  std::string code;

  /// List of lexical errors encountered during tokenization
  std::vector<LexError> errors;
  
  /// Current position in the source string (byte index)
  size_t position = 0;

  /// Current line number
  int line = 1;

  /// Index of the first character of the current line
  size_t lineStart = 0;

  /// @internal
  /// Returns true if the lexer has reached the end of the input.
  bool endOfFile() const;

  /// @internal
  /// Returns the character at the current position plus an optional offset
  /// without advancing the input position.
  ///
  /// @param offset Lookahead offset from the current position.
  /// @return The requested character.
  char peek(size_t offset = 0) const;

  /// @internal
  /// Returns the current character and advances the input position.
  ///
  /// Updates line and column tracking as needed.
  /// @return The consumed character.
  char advance();
  
  /// @internal
  /// Records a lexical error at the given source span.
  ///
  /// The error is stored internally and does not interrupt tokenization.
  ///
  /// @param span Source span associated with the error.
  /// @param msg Human-readable error message.
  void addError(const SourceSpan& span, const std::string &msg);

  /// @internal
  /// Skips whitespace characters and updates line counters.
  void skipWhitespace();

  /// @internal
  /// Skips a comment starting at the current position.
  ///
  /// Comment syntax is defined by the lexer implementation
  /// (e.g. single-line comments).
  void skipComment();
  
  /// @internal
  /// Reads an identifier or instruction mnemonic.
  ///
  /// @return Token of type TokenType::IDENTIFIER.
  Token readIdentifier();

  /// @internal
  /// Reads a string literal.
  ///
  /// @return Token of type TokenType::STRING.
  Token readString();

  /// @internal
  /// Reads a numeric literal.
  ///
  /// @return Token of type TokenType::NUMBER.
  Token readNumber();

public:
  /// Constructs a lexer for the given source code.
  ///
  /// @param source Full assembly source code to be tokenized.
  /// @note No tokenization is performed during construction.
  ///       Call tokenize() to start lexical analysis.
  explicit Lexer(std::string source);

  /// Tokenizes the input source code.
  ///
  /// Converts the input text into a sequence of tokens in source order.
  /// The lexer attempts to continue tokenization even after encountering
  /// lexical errors.
  ///
  /// @return Vector of tokens produced from the input source.
  std::vector<Token> tokenize();


  /// Returns all lexical errors encountered during tokenization.
  ///
  /// @return Vector of LexError objects.
  std::vector<LexError> getErrors() const;
};

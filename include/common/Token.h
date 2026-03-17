/// @file Token.h
/// @brief Token definitions used by the compilation pipeline.
///
/// Defines token types and token structure produced by the lexer
/// and consumed by subsequent compilation stages (parser, semantic analysis).
#pragma once
#include <string>
#include <variant>

/// @brief Enumeration of all lexical token types.
///
/// TokenType represents the category of a token recognized by the lexer.
/// These tokens form the input stream for the parser.
enum class TokenType {
  /// End of input stream
  END_OF_FILE,

  /// End of current line
  NEWLINE,

  /// Identifier (labels, symbols, instruction mnemonics, mov,..)
  IDENTIFIER,
  
  /// Numeric literal
  NUMBER,

  /// String literal
  STRING,
  
  /// '+'
  PLUS,

  /// '-'
  MINUS,

  /// '*'
  STAR,

  /// '/'
  SLASH,

  /// ','
  COMMA,
  
  /// '.'
  DOT,

  /// ':'
  COLON,

  /// '['
  LBRACKET,

  /// ']'
  RBRACKET,

  /// '('
  LPAREN,

  /// ')'
  RPAREN,

  /// '<<'
  SHIFT_LEFT,

  /// '>>'
  SHIFT_RIGHT,

  /// Invalid or unrecognized token
  INVALID
};

/// @brief Represents a span in the source code.
///
/// SourceSpan describes the exact location of a token in the source text.
/// It is primarily used for diagnostics and error reporting.
struct SourceSpan {
  /// Line number where the token begins
  int line;

  /// Column number where the token begins
  int column;

  /// Length of the token in characters
  int length;
};

/// @brief Represents a single lexical token.
///
/// A token consists of its type, optional semantic value, and source location.
/// Tokens are produced by the lexer and consumed by later compilation stages
/// such as the parser and others.
struct Token {
  /// Token category (identifier, literal, operator, etc.)
  TokenType type;

  /// Source location of the token in the input text
  SourceSpan span;

  /// Token value, depending on its type:
  /// - int64_t for numeric literals
  /// - std::string for identifiers and string literals
  std::variant<int64_t, std::string> value;
};

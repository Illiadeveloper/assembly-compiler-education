#pragma once
#include <string>

enum class TokenType {
  WORD,
  NUMBER,
  COMMA,
  LBRACKET,
  RBRACKET,
  COLON,
  PLUS,
  MINUS,
  STAR,
  INVALID
};

struct Token {
  TokenType type;
  std::string value;
  int line;
};

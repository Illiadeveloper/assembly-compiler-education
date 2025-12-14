#pragma once

#include <string>
#include <vector>

enum class TokenType {
  WORD,
  NUMBER,
  COMMA,
  LBRACKET,
  RBRACKET,
  COLON,
  INVALID
};

struct Token {
  TokenType type;
  std::string value;
  int line;
};

struct LexError {
  int line;
  size_t pos;
  std::string message;
};

class Lexer {
private:
  std::string code;
  std::vector<LexError> errors;
  size_t pos = 0;
  int line = 1;

  bool eof() const;
  char peek() const;
  char peekNext() const;
  char get();

  void addError(const std::string &msg);

  void skipWhitespace();
  void skipComment();
  Token readWord();
  Token readNumber();

public:
  Lexer(const std::string &source);
  std::vector<Token> tokenize();
};

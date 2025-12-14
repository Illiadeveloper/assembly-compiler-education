#include "Lexer.h"
#include <cctype>
#include <iostream>
#include <ratio>

Lexer::Lexer(const std::string &source) : code(source) {}

bool Lexer::eof() const { return pos >= code.size(); }

// Just show the char
char Lexer::peek() const {
  if (pos >= code.size())
    return '\0';
  return code[pos];
}

char Lexer::peekNext() const {
  if (pos + 1 >= code.size())
    return '\0';
  return code[pos + 1];
}

// Get and move
char Lexer::get() {
  if (pos >= code.size()) {
    return '\0';
  }

  char c = code[pos++];
  if (c == '\n')
    line++;
  return c;
}

void Lexer::skipWhitespace() {
  while (std::isspace(peek())) {
    get();
  }
}

void Lexer::skipComment() {
  while (peek() != '\n' && peek() != '\0') {
    // std::cout << "SKIP COMM: " << pos << std::endl;
    get();
  }
}

void Lexer::addError(const std::string &msg) {
  errors.push_back({line, pos, msg});
}

Token Lexer::readWord() {
  int start = pos;
  int startLine = line;
  while (std::isalnum(peek()) || peek() == '_')
    get();
  return {TokenType::WORD, code.substr(start, pos - start), startLine};
}

Token Lexer::readNumber() {
  int start = pos;
  int startLine = line;
  bool isHex = false;

  if (peek() == '0' && (peekNext() == 'x' || peekNext() == 'X')) {
    get();
    get();
    isHex = true;
    if (!std::isxdigit(peek())) {
      size_t end = pos;
      addError("Hex literal '0x' without digits");
      return {TokenType::INVALID, code.substr(start, end - start), startLine};
    }
    while (std::isxdigit(peek()))
      get();
  } else {
    while (std::isdigit(peek()))
      get();
  }

  if (std::isalpha(peek()) || peek() == '_') {
    while (std::isalpha(peek()) || peek() == '_')
      get();
    size_t end = pos;
    addError("Invalid number leteral (letters after digits)");
    return {TokenType::INVALID, code.substr(start, end - start), startLine};
  }

  size_t end = pos;
  return {TokenType::NUMBER, code.substr(start, end - start), startLine};
}

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;

  while (!eof()) {
    skipWhitespace();
    char c = peek();
    if (c == '\0') {
      break;
    } else if (c == ';') {
      skipComment();
    } else if (std::isalpha(c) || c == '_') {
      tokens.push_back(readWord());
    } else if (std::isdigit(c)) {
      tokens.push_back(readNumber());
    } else {
      switch (c) {
      default:
        addError("Unknown symbol");
        get();
        break;
      case ',':
        tokens.push_back({TokenType::COMMA, std::string(1, get()), line});
        break;
      case ':':
        tokens.push_back({TokenType::COLON, std::string(1, get()), line});
        break;
      case '[':
        tokens.push_back({TokenType::LBRACKET, std::string(1, get()), line});
        break;
      case ']':
        tokens.push_back({TokenType::RBRACKET, std::string(1, get()), line});
        break;
      case '+':
        tokens.push_back({TokenType::STAR, std::string(1, get()), line});
        break;
      case '-':
        tokens.push_back({TokenType::MINUS, std::string(1, get()), line});
        break;
      case '*':
        tokens.push_back({TokenType::PLUS, std::string(1, get()), line});
        break;
      }
    }
  }
  return tokens;
}

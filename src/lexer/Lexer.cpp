#include "lexer/Lexer.h"

#include <cctype>
#include <string>

#include "common/CompilerError.h"
#include "common/Token.h"

Lexer::Lexer(std::string source) : code(std::move(source)) {}

bool Lexer::endOfFile() const { return position >= code.size(); }

// Just show the char
char Lexer::peek(size_t offset) const {
  if (position + offset >= code.size()) return '\0';
  return code[position + offset];
}

char Lexer::advance() {
  if (position >= code.size()) {
    return '\0';
  }

  char c = code[position++];
  if (c == '\n') {
    lineStart = position;
    line++;
  }
  return c;
}

void Lexer::skipWhitespace() {
  while (true) {
    char c = peek();
    if (c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v') {
      advance();
    } else {
      break;
    }
  }
}

void Lexer::skipComment() {
  while (peek() != '\n' && peek() != '\0') {
    advance();
  }
}

bool Lexer::hasErrors() const noexcept { return errors.size() > 0; }

void Lexer::addError(const SourceSpan& span, const std::string& msg) {
  errors.push_back({ErrorStage::LEXER, span, msg});
}

const std::vector<CompilerError>& Lexer::getErrors() const { return errors; }

Token Lexer::readIdentifier() {
  int start = position;
  while (std::isalnum(peek()) || peek() == '_') advance();
  return {TokenType::IDENTIFIER,
          {line, static_cast<int>(start - lineStart + 1),
           static_cast<int>(position - start)},
          code.substr(start, position - start)};
}

Token Lexer::readNumber() {
  int start = position;
  int startLine = line;
  bool isHex = false;

  if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'X')) {
    advance();
    advance();
    isHex = true;
    if (!std::isxdigit(peek())) {
      SourceSpan span{line, static_cast<int>(start - lineStart + 1),
                      static_cast<int>(position - start)};
      addError(span, "Hex literal '0x' without digits");
      return {TokenType::INVALID, span, code.substr(start, position - start)};
    }
    while (std::isxdigit(peek())) advance();
  } else {
    while (std::isdigit(peek())) advance();
  }

  if (std::isalpha(peek()) || peek() == '_') {
    while (std::isalpha(peek()) || peek() == '_') advance();
    SourceSpan span{line, static_cast<int>(start - lineStart + 1),
                    static_cast<int>(position - start)};
    addError(span, "Invalid number leteral (letters after digits)");
    return {TokenType::INVALID, span, code.substr(start, position - start)};
  }

  int base = isHex ? 16 : 10;

  return {
      TokenType::NUMBER,
      {line, static_cast<int>(start - lineStart + 1),
       static_cast<int>(position - start)},
      std::stoll(code.substr(start, position - start).c_str(), nullptr, base)};
}

Token Lexer::readString() {
  int start = position;
  advance();

  std::string value;

  while (!endOfFile()) {
    char currentChar = peek();
    if (currentChar == '"') {
      advance();
      return {TokenType::STRING,
              {line, static_cast<int>(start - lineStart + 1),
               static_cast<int>(position - start)},
              value};
    }
    if (currentChar == '\\') {
      advance();
      if (endOfFile()) break;

      char esc = peek();
      switch (esc) {
        case 'n':
          value.push_back('\n');
          break;
        case 't':
          value.push_back('\t');
          break;
        case '"':
          value.push_back('\"');
          break;
        case '\\':
          value.push_back('\\');
          break;
        default:
          addError({line, static_cast<int>(position), 1},
                   "Unknown escape sequence.");
          value.push_back(esc);
          break;
      }
      advance();
    } else {
      value.push_back(currentChar);
      advance();
    }
  }

  SourceSpan span{line, static_cast<int>(start - lineStart + 1),
                  static_cast<int>(position - start)};
  addError(span, "Unterminated string literal");

  return {TokenType::INVALID, span, value};
}

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;

  while (!endOfFile()) {
    skipWhitespace();

    if (endOfFile()) break;

    char currentChar = peek();

    if (currentChar == '\n') {
      int startColumn = static_cast<int>(position - lineStart - 1);
      advance();
      SourceSpan span{line, startColumn, 1};
      tokens.push_back({TokenType::NEWLINE, span, std::string("\n")});
      continue;
    }

    if (currentChar == ';') {
      skipComment();
      continue;
    }

    if (currentChar == '"') {
      tokens.push_back(readString());
      continue;
    }

    if (std::isalpha(currentChar) || currentChar == '_') {
      tokens.push_back(readIdentifier());
      continue;
    }

    if (std::isdigit(currentChar)) {
      tokens.push_back(readNumber());
      continue;
    }

    int startColumn = static_cast<int>(position - lineStart + 1);

    switch (currentChar) {
      case ',':
        advance();
        tokens.push_back(
            {TokenType::COMMA, {line, startColumn, 1}, std::string(",")});
        break;

      case '.':
        advance();
        tokens.push_back(
            {TokenType::DOT, {line, startColumn, 1}, std::string(",")});
        break;

      case ':':
        advance();
        tokens.push_back(
            {TokenType::COLON, {line, startColumn, 1}, std::string(":")});
        break;

      case '[':
        advance();
        tokens.push_back(
            {TokenType::LBRACKET, {line, startColumn, 1}, std::string("[")});
        break;

      case ']':
        advance();
        tokens.push_back(
            {TokenType::RBRACKET, {line, startColumn, 1}, std::string("]")});
        break;

      case '+':
        advance();
        tokens.push_back(
            {TokenType::PLUS, {line, startColumn, 1}, std::string("+")});
        break;

      case '-':
        advance();
        tokens.push_back(
            {TokenType::MINUS, {line, startColumn, 1}, std::string("-")});
        break;

      case '*':
        advance();
        tokens.push_back(
            {TokenType::STAR, {line, startColumn, 1}, std::string("*")});
        break;

      case '/':
        advance();
        tokens.push_back(
            {TokenType::SLASH, {line, startColumn, 1}, std::string("/")});
        break;
      default:
        advance();
        SourceSpan span{line, startColumn, 1};
        addError(span, "Unknown symbol");
        tokens.push_back(
            {TokenType::INVALID, span, std::string(1, currentChar)});
        break;
    }
  }
  tokens.push_back({TokenType::END_OF_FILE,
                    {line, static_cast<int>(position - lineStart + 1), 0},
                    ""});
  return tokens;
}

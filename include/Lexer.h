#include <string>
#include <vector>

enum class TokenType {
    WORD,
    NUMBER,
    COMMA,
    LABEL,
    LBRACKET,
    RBRACKET,
    COMMENT
};

struct Token {
  TokenType type;
  std::string value;
  int line;
};

class Lexer {
private:
  std::string code;
  size_t pos = 0;
  int line = 1;

  char peek();
  char get();
  void skipWhitespace();
  void skipComment();
  Token readWord();
  Token readNumber();

public:
  Lexer(const std::string& source);
  std::vector<Token> tokenize();
};

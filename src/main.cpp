#include "lexer/Lexer.h"
#include <iostream>
#include <vector>

void test(const std::string &src) {
  std::cout << "=== SOURCE ===\n" << src << "\n";
  Lexer lex(src);
  auto tokens = lex.tokenize();

  std::cout << "--- TOKENS ---\n";
  for (const auto &t : tokens) {
    std::cout << "TYPE: " << (int)t.type << "\tVALUE: '" << t.value
              << "'\tLINE: " << t.line << "\n";
  }
  std::cout << "\n";
}

int main() {
  test("MOV RAX, EAX");
  test("label:\n  MOV RAX, [RBX]");
  test("ADD RAX, 10");
  test("MOV RAX, 0xFF");
  test("MOV RAX, 8aaa");
  test("MOV RAX, 5dm29");
  test("MOV RAX, [RBX+4]");
  test("; this is a comment\nMOV RAX, EAX");

  return 0;
}

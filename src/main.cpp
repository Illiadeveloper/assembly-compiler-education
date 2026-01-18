#include "lexer/Lexer.h"
#include <iostream>
#include <string>
#include <vector>

void test(const std::string &src) {
  std::cout << "=== SOURCE ===\n" << src << "\n";
  Lexer lex(src);
  auto tokens = lex.tokenize();

  std::cout << "--- TOKENS ---\n";
  for (const auto &t : tokens) {
    std::cout << "TYPE: " << (int)t.type << "\tVALUE: '";
    if (auto *v = std::get_if<int64_t>(&t.value)) {
      std::cout << *v;
    } else {
      std::cout << *std::get_if<std::string>(&t.value);
    }
    std::cout << "'\tLINE: " << t.span.line << "\n";
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

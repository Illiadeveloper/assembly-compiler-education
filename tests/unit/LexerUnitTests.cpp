#include "lexer/Lexer.h"
#include <algorithm>
#include <gtest/gtest.h>

static std::vector<TokenType> types(const std::vector<Token> &toks) {
  std::vector<TokenType> out;
  out.reserve(toks.size());
  for (const auto &t : toks)
    out.push_back(t.type);
  return out;
}

static std::vector<std::string> values(const std::vector<Token> &toks) {
  std::vector<std::string> out;
  out.reserve(toks.size());
  for (const auto &t : toks)
    out.push_back(t.value);
  return out;
}

TEST(TokenStruct, FieldsAccessibleAndCorrect) {
  Token t{TokenType::WORD, "identifier", 42};
  EXPECT_EQ(t.type, TokenType::WORD);
  EXPECT_EQ(t.value, "identifier");
  EXPECT_EQ(t.line, 42);
}

TEST(Lexer_Assembler, LabelAndOperands) {
  const std::string src = "start: MOV R1, 42";
  Lexer lx(src);
  auto toks = lx.tokenize();
  auto errs = lx.getErrors();

  EXPECT_TRUE(errs.empty());

  std::vector<TokenType> expected_types = {
      TokenType::WORD,  // start
      TokenType::COLON, // :
      TokenType::WORD,  // MOV
      TokenType::WORD,  // R1
      TokenType::COMMA, // ,
      TokenType::NUMBER // 42
  };

  std::vector<std::string> expected_values = {"start", ":", "MOV",
                                              "R1",    ",", "42"};

  EXPECT_EQ(toks.size(), expected_types.size());
  EXPECT_EQ(types(toks), expected_types);
  EXPECT_EQ(values(toks), expected_values);
}

TEST(Lexer_Assembler, CommentFullLine_Semicolon_Skipped) {
  const std::string src = "; this is a comment line\nMOV R1, R2";
  Lexer lx(src);
  auto toks = lx.tokenize();
  auto errs = lx.getErrors();

  EXPECT_TRUE(errs.empty());

  std::vector<TokenType> expected_types = {
      TokenType::WORD,  // MOV
      TokenType::WORD,  // R1
      TokenType::COMMA, // ,
      TokenType::WORD   // R2
  };

  EXPECT_EQ(types(toks), expected_types);
}

TEST(Lexer_Assembler, Comment_Ignored) {
  const std::string src = "ADD R3, R4 ; add registers";
  Lexer lx(src);
  auto toks = lx.tokenize();
  auto errs = lx.getErrors();

  EXPECT_TRUE(errs.empty());
  std::vector<std::string> expected_values = {"ADD", "R3", ",", "R4"};
  EXPECT_EQ(values(toks), expected_values);
}

TEST(Lexer_Assembler, Brackets_MemoryOperand) {
  const std::string src = "LD [R1], R2";
  Lexer lx(src);
  auto toks = lx.tokenize();
  auto errs = lx.getErrors();

  EXPECT_TRUE(errs.empty());

  std::vector<TokenType> expected_types = {
      TokenType::WORD,     // LD
      TokenType::LBRACKET, // [
      TokenType::WORD,     // R1
      TokenType::RBRACKET, // ]
      TokenType::COMMA,    // ,
      TokenType::WORD      // R2
  };

  EXPECT_EQ(types(toks), expected_types);
}

TEST(Lexer_Assembler, InvalidCharacter_ProducesErrorAndInvalidToken) {
  const std::string src = "@illegal #also";
  Lexer lx(src);
  auto toks = lx.tokenize();
  auto errs = lx.getErrors();

  EXPECT_FALSE(errs.empty());
  bool has_invalid = std::any_of(toks.begin(), toks.end(), [](const Token &t) {
    std::cout << (int)t.type << std::endl;
    return t.type == TokenType::INVALID;
  });
  EXPECT_TRUE(has_invalid);
}

TEST(Lexer_Assembler, LineNumbers_WithCommentsAndEmptyLines) {
  const std::string src = "label:\n; full comment\nMOV R1, 1 ; inline\n\n; "
                          "another comment\nJMP label";
  Lexer lx(src);
  auto toks = lx.tokenize();
  auto errs = lx.getErrors();

  EXPECT_TRUE(errs.empty());

  bool label_on_1 = false;
  bool mov_on_3 = false;
  bool jmp_on_6 = false;
  for (const auto &t : toks) {
    if (t.value == "label" && t.line == 1)
      label_on_1 = true;
    if (t.value == "MOV" && t.line == 3)
      mov_on_3 = true;
    if (t.value == "JMP" && t.line == 6)
      jmp_on_6 = true;
  }

  EXPECT_TRUE(label_on_1);
  EXPECT_TRUE(mov_on_3);
  EXPECT_TRUE(jmp_on_6);
}

TEST(Lexer_NoErrorsForValidAssemblerInput, EmptyErrorsVector) {
  const std::string src = "START: MOV R0, 0\nLD [R2], R3\nADD R1, R2";
  Lexer lx(src);
  lx.tokenize();
  auto errs = lx.getErrors();
  EXPECT_TRUE(errs.empty());
}

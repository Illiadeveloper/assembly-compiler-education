
#include "common/Token.h"
#include "lexer/Lexer.h"
#include <algorithm>
#include <gtest/gtest.h>
#include <variant>

// ===================== helpers =====================

static std::vector<Token> withoutEOF(const std::vector<Token> &toks) {
  if (!toks.empty() && toks.back().type == TokenType::END_OF_FILE) {
    return {toks.begin(), toks.end() - 1};
  }
  return toks;
}

static std::vector<TokenType> types(const std::vector<Token> &toks) {
  std::vector<TokenType> out;
  out.reserve(toks.size());
  for (const auto &t : toks) {
    // std::cout << (int)t.type << std::endl;
    out.push_back(t.type);
  }
  return out;
}

static std::vector<TokenType> typesNoEOF(const std::vector<Token> &toks) {
  return types(withoutEOF(toks));
}

static std::vector<std::string> values(const std::vector<Token> &toks) {
  std::vector<std::string> out;
  out.reserve(toks.size());
  for (const auto &t : toks) {
    if (std::holds_alternative<std::string>(t.value)) {
      out.push_back(std::get<std::string>(t.value));
    } else if (std::holds_alternative<int64_t>(t.value)) {
      out.push_back(std::to_string(std::get<int64_t>(t.value)));
    } else {
      out.push_back({});
    }
  }
  return out;
}

static std::vector<std::string> valuesNoEOF(const std::vector<Token> &toks) {
  return values(withoutEOF(toks));
}

// ===================== tests =====================

TEST(TokenStruct, FieldsAccessibleAndCorrect) {
  Token t{TokenType::IDENTIFIER, SourceSpan{42, 5, 10},
          std::string("identifier")};

  EXPECT_EQ(t.type, TokenType::IDENTIFIER);
  EXPECT_EQ(t.span.line, 42);
  EXPECT_EQ(t.span.column, 5);
  EXPECT_EQ(t.span.length, 10);

  ASSERT_TRUE(std::holds_alternative<std::string>(t.value));
  EXPECT_EQ(std::get<std::string>(t.value), "identifier");
}

TEST(Lexer, EndsWithEOF) {
  Lexer lx("MOV R1, 1");
  auto toks = lx.tokenize();

  ASSERT_FALSE(toks.empty());
  EXPECT_EQ(toks.back().type, TokenType::END_OF_FILE);
}

TEST(Lexer_Assembler, LabelAndOperands) {
  const std::string src = "start: MOV R1, 42";
  Lexer lx(src);

  auto toks = lx.tokenize();
  auto errs = lx.getErrors();

  EXPECT_TRUE(errs.empty());
  EXPECT_EQ(toks.back().type, TokenType::END_OF_FILE);

  std::vector<TokenType> expected_types = {
      TokenType::IDENTIFIER, // start
      TokenType::COLON,      // :
      TokenType::IDENTIFIER, // MOV
      TokenType::IDENTIFIER, // R1
      TokenType::COMMA,      // ,
      TokenType::NUMBER      // 42
  };

  EXPECT_EQ(typesNoEOF(toks), expected_types);

  auto vals = valuesNoEOF(toks);
  EXPECT_EQ(vals[0], "start");
  EXPECT_EQ(vals[2], "MOV");
  EXPECT_EQ(vals[3], "R1");
  EXPECT_EQ(vals[5], "42");
}

TEST(Lexer_Assembler, CommentFullLine_Semicolon_Skipped) {
  const std::string src = "; comment line\nMOV R1, R2";
  Lexer lx(src);

  auto toks = lx.tokenize();
  auto errs = lx.getErrors();

  EXPECT_TRUE(errs.empty());
  EXPECT_EQ(toks.back().type, TokenType::END_OF_FILE);

  std::vector<TokenType> expected_types = {
      TokenType::NEWLINE,    // \n
      TokenType::IDENTIFIER, // MOV
      TokenType::IDENTIFIER, // R1
      TokenType::COMMA,      // ,
      TokenType::IDENTIFIER, // R2
  };

  EXPECT_EQ(typesNoEOF(toks), expected_types);
}

TEST(Lexer_Assembler, Comment_Ignored) {
  const std::string src = "ADD R3, R4 ; add registers";
  Lexer lx(src);

  auto toks = lx.tokenize();
  auto errs = lx.getErrors();

  EXPECT_TRUE(errs.empty());
  EXPECT_EQ(toks.back().type, TokenType::END_OF_FILE);

  std::vector<std::string> expected_values = {"ADD", "R3", ",", "R4"};

  EXPECT_EQ(valuesNoEOF(toks), expected_values);
}

TEST(Lexer_Assembler, Brackets_MemoryOperand) {
  const std::string src = "LD [R1], R2";
  Lexer lx(src);

  auto toks = lx.tokenize();
  auto errs = lx.getErrors();

  EXPECT_TRUE(errs.empty());
  EXPECT_EQ(toks.back().type, TokenType::END_OF_FILE);

  std::vector<TokenType> expected_types = {
      TokenType::IDENTIFIER, // LD
      TokenType::LBRACKET,   // [
      TokenType::IDENTIFIER, // R1
      TokenType::RBRACKET,   // ]
      TokenType::COMMA,      // ,
      TokenType::IDENTIFIER  // R2
  };

  EXPECT_EQ(typesNoEOF(toks), expected_types);
}

TEST(Lexer_Assembler, InvalidCharacter_ProducesErrorAndInvalidToken) {
  const std::string src = "@illegal #also";
  Lexer lx(src);

  auto toks = lx.tokenize();
  auto errs = lx.getErrors();

  EXPECT_FALSE(errs.empty());
  EXPECT_EQ(toks.back().type, TokenType::END_OF_FILE);

  bool has_invalid =
      std::any_of(toks.begin(), toks.end() - 1,
                  [](const Token &t) { return t.type == TokenType::INVALID; });

  EXPECT_TRUE(has_invalid);
}

TEST(Lexer_Assembler, LineNumbers_WithCommentsAndEmptyLines) {
  const std::string src = "label:\n"
                          "; full comment\n"
                          "MOV R1, 1 ; inline\n"
                          "\n"
                          "; another comment\n"
                          "JMP label";

  Lexer lx(src);
  auto toks = lx.tokenize();
  auto errs = lx.getErrors();

  EXPECT_TRUE(errs.empty());
  EXPECT_EQ(toks.back().type, TokenType::END_OF_FILE);

  bool label_on_1 = false;
  bool mov_on_3 = false;
  bool jmp_on_6 = false;

  for (const auto &t : toks) {
    if (t.type == TokenType::END_OF_FILE)
      continue;

    if (!std::holds_alternative<std::string>(t.value))
      continue;

    const auto &v = std::get<std::string>(t.value);

    if (v == "label" && t.span.line == 1)
      label_on_1 = true;
    if (v == "MOV" && t.span.line == 3)
      mov_on_3 = true;
    if (v == "JMP" && t.span.line == 6)
      jmp_on_6 = true;
  }

  EXPECT_TRUE(label_on_1);
  EXPECT_TRUE(mov_on_3);
  EXPECT_TRUE(jmp_on_6);
}

TEST(Lexer_NoErrorsForValidAssemblerInput, EmptyErrorsVector) {
  const std::string src = "START: MOV R0, 0\n"
                          "LD [R2], R3\n"
                          "ADD R1, R2";

  Lexer lx(src);
  lx.tokenize();

  auto errs = lx.getErrors();
  EXPECT_TRUE(errs.empty());
}

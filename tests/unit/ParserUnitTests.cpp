#include <gtest/gtest.h>

#include "ast/Directive.h"
#include "ast/Instruction.h"
#include "ast/Statement.h"
#include "common/Registers.h"
#include "common/Token.h"
#include "parser/Parser.h"

#include <variant>

// ============================================================
//  Helpers
// ============================================================

static SourceSpan make_span(int line = 1, int col = 1, int length = 1) {
  return SourceSpan{line, col, length};
}

static std::string formatErrors(const Parser &p) {
  std::ostringstream out;
  for (const auto &e : p.getErrors())
    out << "Error at line " << e.span.line << ":" << e.span.column << " -> "
        << e.message << "\n";
  return out.str();
}

static Token make_ident(const std::string &s, int line = 1, int col = 1) {
  Token t;
  t.type = TokenType::IDENTIFIER;
  t.span = make_span(line, col, (int)s.size());
  t.value = s;
  return t;
}

static Token make_number(int64_t v, int line = 1, int col = 1) {
  Token t;
  t.type = TokenType::NUMBER;
  t.span = make_span(line, col, 0);
  t.value = v;
  return t;
}

static Token make_string(const std::string &s, int line = 1, int col = 1) {
  Token t;
  t.type = TokenType::STRING;
  t.span = make_span(line, col, (int)s.size() + 2);
  t.value = s;
  return t;
}

static Token make_simple(TokenType type, int line = 1, int col = 1) {
  Token t;
  t.type = type;
  t.span = make_span(line, col, 1);
  switch (type) {
  case TokenType::PLUS:
    t.value = std::string("+");
    break;
  case TokenType::MINUS:
    t.value = std::string("-");
    break;
  case TokenType::STAR:
    t.value = std::string("*");
    break;
  case TokenType::SLASH:
    t.value = std::string("/");
    break;
  case TokenType::SHIFT_LEFT:
    t.value = std::string("<<");
    break;
  case TokenType::SHIFT_RIGHT:
    t.value = std::string(">>");
    break;
  case TokenType::COMMA:
    t.value = std::string(",");
    break;
  case TokenType::COLON:
    t.value = std::string(":");
    break;
  case TokenType::LBRACKET:
    t.value = std::string("[");
    break;
  case TokenType::RBRACKET:
    t.value = std::string("]");
    break;
  case TokenType::LPAREN:
    t.value = std::string("(");
    break;
  case TokenType::RPAREN:
    t.value = std::string(")");
    break;
  case TokenType::DOT:
    t.value = std::string(".");
    break;
  case TokenType::NEWLINE:
    t.value = std::string("\n");
    break;
  default:
    t.value = std::string();
  }
  return t;
}

static Token make_eof(int line = 1, int col = 1) {
  Token t;
  t.type = TokenType::END_OF_FILE;
  t.span = make_span(line, col, 0);
  t.value = std::string();
  return t;
}

// ============================================================
//  Label tests
// ============================================================

// Label immediately followed by an instruction (no blank line between them)
TEST(ParserLabel, LabelFollowedByInstruction) {
  // start: nop\n
  std::vector<Token> toks = {make_ident("start"), make_simple(TokenType::COLON),
                             make_ident("nop"), make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  ASSERT_EQ(prog.size(), 2u);
  ASSERT_TRUE(std::holds_alternative<Label>(prog[0]));
  EXPECT_EQ(std::get<Label>(prog[0]).name, "start");
  EXPECT_TRUE(std::holds_alternative<Instruction>(prog[1]));
}

// Two labels in a row (common aliasing pattern)
TEST(ParserLabel, TwoConsecutiveLabels) {
  // foo:\n bar:\n
  std::vector<Token> toks = {make_ident("foo"),
                             make_simple(TokenType::COLON),
                             make_simple(TokenType::NEWLINE),
                             make_ident("bar"),
                             make_simple(TokenType::COLON),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  ASSERT_EQ(prog.size(), 2u);
  EXPECT_EQ(std::get<Label>(prog[0]).name, "foo");
  EXPECT_EQ(std::get<Label>(prog[1]).name, "bar");
}

// ============================================================
//  Instruction / operand tests
// ============================================================

// Zero-operand instruction
TEST(ParserInstruction, ZeroOperands) {
  // ret\n
  std::vector<Token> toks = {make_ident("ret"), make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  ASSERT_EQ(prog.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<Instruction>(prog[0]));
  EXPECT_EQ(std::get<Instruction>(prog[0]).operands.size(), 0u);
}

// Single immediate operand
TEST(ParserInstruction, PushImmediate) {
  // push 1\n
  std::vector<Token> toks = {make_ident("push"), make_number(1),
                             make_simple(TokenType::NEWLINE), make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  ASSERT_EQ(prog.size(), 1u);
  const Instruction &ins = std::get<Instruction>(prog[0]);
  ASSERT_EQ(ins.operands.size(), 1u);
  EXPECT_TRUE(ins.operands[0].isImm());
  EXPECT_EQ(ins.operands[0].getImm(), 1);
}

// Negative immediate: add rax, -1
TEST(ParserInstruction, NegativeImmediate) {
  std::vector<Token> toks = {make_ident("add"),
                             make_ident("rax"),
                             make_simple(TokenType::COMMA),
                             make_simple(TokenType::MINUS),
                             make_number(1),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  const Instruction &ins = std::get<Instruction>(prog[0]);
  ASSERT_EQ(ins.operands.size(), 2u);
  EXPECT_TRUE(ins.operands[1].isImm());
  EXPECT_EQ(ins.operands[1].getImm(), -1);
}

// Memory as destination: mov [rax], rbx
TEST(ParserInstruction, MemoryDestination) {
  std::vector<Token> toks = {make_ident("mov"),
                             make_simple(TokenType::LBRACKET),
                             make_ident("rax"),
                             make_simple(TokenType::RBRACKET),
                             make_simple(TokenType::COMMA),
                             make_ident("rbx"),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  const Instruction &ins = std::get<Instruction>(prog[0]);
  ASSERT_EQ(ins.operands.size(), 2u);
  EXPECT_TRUE(ins.operands[0].isMem());
  EXPECT_TRUE(ins.operands[1].isReg());
  EXPECT_EQ(ins.operands[0].getMem().base.value(), Register::RAX);
}

// Three operands: imul rcx, rdx, 4
TEST(ParserInstruction, ThreeOperands) {
  std::vector<Token> toks = {make_ident("imul"),
                             make_ident("rcx"),
                             make_simple(TokenType::COMMA),
                             make_ident("rdx"),
                             make_simple(TokenType::COMMA),
                             make_number(4),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  const Instruction &ins = std::get<Instruction>(prog[0]);
  ASSERT_EQ(ins.operands.size(), 3u);
  EXPECT_TRUE(ins.operands[0].isReg());
  EXPECT_EQ(ins.operands[0].getReg(), Register::RCX);
  EXPECT_TRUE(ins.operands[1].isReg());
  EXPECT_EQ(ins.operands[1].getReg(), Register::RDX);
  EXPECT_TRUE(ins.operands[2].isImm());
  EXPECT_EQ(ins.operands[2].getImm(), 4);
}

// Label as branch operand: jmp loop_start
TEST(ParserInstruction, LabelOperand) {
  std::vector<Token> toks = {make_ident("jmp"), make_ident("loop_start"),
                             make_simple(TokenType::NEWLINE), make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  const Instruction &ins = std::get<Instruction>(prog[0]);
  ASSERT_EQ(ins.operands.size(), 1u);
  EXPECT_TRUE(ins.operands[0].isLabel());
  EXPECT_EQ(ins.operands[0].getLabel(), "loop_start");
}

// Size prefix: mov qword ptr [rax], rbx
TEST(ParserInstruction, QwordSizePrefix) {
  std::vector<Token> toks = {make_ident("mov"),
                             make_ident("qword"),
                             make_ident("ptr"),
                             make_simple(TokenType::LBRACKET),
                             make_ident("rax"),
                             make_simple(TokenType::RBRACKET),
                             make_simple(TokenType::COMMA),
                             make_ident("rbx"),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  const Instruction &ins = std::get<Instruction>(prog[0]);
  ASSERT_EQ(ins.operands.size(), 2u);
  EXPECT_TRUE(ins.operands[0].isMem());
  EXPECT_EQ(ins.operands[0].size, OperandSize::B64);
}

// ============================================================
//  Memory operand expression tests
// ============================================================

// Base only: [rsp]
TEST(ParserMemoryOperand, BaseOnly) {
  std::vector<Token> toks = {make_ident("push"),
                             make_simple(TokenType::LBRACKET),
                             make_ident("rsp"),
                             make_simple(TokenType::RBRACKET),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  const MemoryOperand &m = std::get<Instruction>(prog[0]).operands[0].getMem();
  ASSERT_TRUE(m.base.has_value());
  EXPECT_EQ(m.base.value(), Register::RSP);
  EXPECT_FALSE(m.index.has_value());
  EXPECT_EQ(m.displacement, 0);
}

// Base + index: [rax + rcx]
TEST(ParserMemoryOperand, BaseAndIndex) {
  std::vector<Token> toks = {make_ident("mov"),
                             make_ident("rdx"),
                             make_simple(TokenType::COMMA),
                             make_simple(TokenType::LBRACKET),
                             make_ident("rax"),
                             make_simple(TokenType::PLUS),
                             make_ident("rcx"),
                             make_simple(TokenType::RBRACKET),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  const MemoryOperand &m = std::get<Instruction>(prog[0]).operands[1].getMem();
  EXPECT_EQ(m.base.value(), Register::RAX);
  ASSERT_TRUE(m.index.has_value());
  EXPECT_EQ(m.index.value(), Register::RCX);
  EXPECT_EQ(m.scale, 1);
  EXPECT_EQ(m.displacement, 0);
}

// SIB + displacement: [rax + rcx*4 + 16]
TEST(ParserMemoryOperand, SIBWithDisplacement) {
  std::vector<Token> toks = {make_ident("mov"),
                             make_ident("rdx"),
                             make_simple(TokenType::COMMA),
                             make_simple(TokenType::LBRACKET),
                             make_ident("rax"),
                             make_simple(TokenType::PLUS),
                             make_ident("rcx"),
                             make_simple(TokenType::STAR),
                             make_number(4),
                             make_simple(TokenType::PLUS),
                             make_number(16),
                             make_simple(TokenType::RBRACKET),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  const MemoryOperand &m = std::get<Instruction>(prog[0]).operands[1].getMem();
  EXPECT_EQ(m.base.value(), Register::RAX);
  ASSERT_TRUE(m.index.has_value());
  EXPECT_EQ(m.index.value(), Register::RCX);
  EXPECT_EQ(m.scale, 4);
  EXPECT_EQ(m.displacement, 16);
}

// Negative displacement: [rbp - 8]
TEST(ParserMemoryOperand, NegativeDisplacement) {
  std::vector<Token> toks = {make_ident("mov"),
                             make_ident("rax"),
                             make_simple(TokenType::COMMA),
                             make_simple(TokenType::LBRACKET),
                             make_ident("rbp"),
                             make_simple(TokenType::MINUS),
                             make_number(8),
                             make_simple(TokenType::RBRACKET),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  const MemoryOperand &m = std::get<Instruction>(prog[0]).operands[1].getMem();
  EXPECT_EQ(m.base.value(), Register::RBP);
  EXPECT_EQ(m.displacement, -8);
}

// Symbol in memory expression: [buf + rcx]
// 'buf' is not a register -> goes into symbols[], rcx -> index
TEST(ParserMemoryOperand, SymbolInExpression) {
  std::vector<Token> toks = {make_ident("mov"),
                             make_ident("rax"),
                             make_simple(TokenType::COMMA),
                             make_simple(TokenType::LBRACKET),
                             make_ident("buf"),
                             make_simple(TokenType::PLUS),
                             make_ident("rcx"),
                             make_simple(TokenType::RBRACKET),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  const MemoryOperand &m = std::get<Instruction>(prog[0]).operands[1].getMem();
  ASSERT_TRUE(m.base.has_value());
  EXPECT_EQ(m.base.value(), Register::RCX);
  ASSERT_EQ(m.symbols.size(), 1u);
  EXPECT_EQ(m.symbols[0].name, "buf");
  EXPECT_EQ(m.symbols[0].sign, +1);
}

// ============================================================
//  Section directive  (type is SectionDirectiveType enum)
// ============================================================

TEST(ParserSection, ParsesTextSection) {
  std::vector<Token> toks = {make_ident("section"), make_simple(TokenType::DOT),
                             make_ident("text"),
                             make_simple(TokenType::NEWLINE), make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  ASSERT_EQ(prog.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<SectionDirective>(prog[0]));
  EXPECT_EQ(std::get<SectionDirective>(prog[0]).type,
            SectionDirectiveType::TEXT);
}

TEST(ParserSection, ParsesDataSection) {
  std::vector<Token> toks = {make_ident("section"), make_simple(TokenType::DOT),
                             make_ident("data"),
                             make_simple(TokenType::NEWLINE), make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  EXPECT_EQ(std::get<SectionDirective>(prog[0]).type,
            SectionDirectiveType::DATA);
}

TEST(ParserSection, ParsesBssSection) {
  std::vector<Token> toks = {make_ident("section"), make_simple(TokenType::DOT),
                             make_ident("bss"), make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  EXPECT_EQ(std::get<SectionDirective>(prog[0]).type,
            SectionDirectiveType::BSS);
}

// ============================================================
//  Global directive
// ============================================================

TEST(ParserGlobal, ParsesGlobalDirective) {
  std::vector<Token> toks = {make_ident("global"), make_ident("_start"),
                             make_simple(TokenType::NEWLINE), make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  ASSERT_EQ(prog.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<GlobalDirective>(prog[0]));
  EXPECT_EQ(std::get<GlobalDirective>(prog[0]).name, "_start");
}

// ============================================================
//  EQU directive
// ============================================================

TEST(ParserEqu, ParsesEquWithInteger) {
  // BUFSIZE equ 1024\n
  std::vector<Token> toks = {make_ident("BUFSIZE"), make_ident("equ"),
                             make_number(1024), make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  ASSERT_EQ(prog.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<EquDirective>(prog[0]));
  const EquDirective &eq = std::get<EquDirective>(prog[0]);
  EXPECT_EQ(eq.name, "BUFSIZE");
  EXPECT_EQ(eq.value, 1024);
}

TEST(ParserEqu, ParsesEquWithNegativeInteger) {
  // OFFSET equ -16\n
  std::vector<Token> toks = {make_ident("OFFSET"),
                             make_ident("equ"),
                             make_simple(TokenType::MINUS),
                             make_number(16),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  const EquDirective &eq = std::get<EquDirective>(prog[0]);
  EXPECT_EQ(eq.name, "OFFSET");
  EXPECT_EQ(eq.value, -16);
}

// ============================================================
//  Data directives  (db / dw / dd / dq)
// ============================================================

TEST(ParserDataDirective, ParsesDwWithInteger) {
  std::vector<Token> toks = {make_ident("dw"), make_number(1000),
                             make_simple(TokenType::NEWLINE), make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  ASSERT_EQ(prog.size(), 1u);
  const DataDirective &d = std::get<DataDirective>(prog[0]);
  EXPECT_EQ(d.kind, DataDirectiveKind::DW);
  ASSERT_EQ(d.values.size(), 1u);
  EXPECT_EQ(std::get<int64_t>(d.values[0].value), 1000);
}

TEST(ParserDataDirective, ParsesDdWithInteger) {
  std::vector<Token> toks = {make_ident("dd"), make_number(0xDEADBEEF),
                             make_simple(TokenType::NEWLINE), make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  const DataDirective &d = std::get<DataDirective>(prog[0]);
  EXPECT_EQ(d.kind, DataDirectiveKind::DD);
  EXPECT_EQ(std::get<int64_t>(d.values[0].value),
            static_cast<int64_t>(0xDEADBEEF));
}

TEST(ParserDataDirective, ParsesDqWithInteger) {
  std::vector<Token> toks = {make_ident("dq"), make_number(0),
                             make_simple(TokenType::NEWLINE), make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  const DataDirective &d = std::get<DataDirective>(prog[0]);
  EXPECT_EQ(d.kind, DataDirectiveKind::DQ);
  EXPECT_EQ(std::get<int64_t>(d.values[0].value), 0);
}

// Multiple comma-separated values: db 1, 2, 3
TEST(ParserDataDirective, ParsesDbWithMultipleValues) {
  std::vector<Token> toks = {make_ident("db"),
                             make_number(1),
                             make_simple(TokenType::COMMA),
                             make_number(2),
                             make_simple(TokenType::COMMA),
                             make_number(3),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  const DataDirective &d = std::get<DataDirective>(prog[0]);
  EXPECT_EQ(d.kind, DataDirectiveKind::DB);
  ASSERT_EQ(d.values.size(), 3u);
  EXPECT_EQ(std::get<int64_t>(d.values[0].value), 1);
  EXPECT_EQ(std::get<int64_t>(d.values[1].value), 2);
  EXPECT_EQ(std::get<int64_t>(d.values[2].value), 3);
}

// db "hello"
TEST(ParserDataDirective, ParsesDbWithString) {
  std::vector<Token> toks = {make_ident("db"), make_string("hello"),
                             make_simple(TokenType::NEWLINE), make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  const DataDirective &d = std::get<DataDirective>(prog[0]);
  EXPECT_EQ(d.kind, DataDirectiveKind::DB);
  ASSERT_EQ(d.values.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<std::string>(d.values[0].value));
  EXPECT_EQ(std::get<std::string>(d.values[0].value), "hello");
}

// db "hi", 0  — string + null terminator
TEST(ParserDataDirective, ParsesDbStringWithNullTerminator) {
  std::vector<Token> toks = {make_ident("db"),
                             make_string("hi"),
                             make_simple(TokenType::COMMA),
                             make_number(0),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  const DataDirective &d = std::get<DataDirective>(prog[0]);
  ASSERT_EQ(d.values.size(), 2u);
  EXPECT_TRUE(std::holds_alternative<std::string>(d.values[0].value));
  ASSERT_TRUE(std::holds_alternative<int64_t>(d.values[1].value));
  EXPECT_EQ(std::get<int64_t>(d.values[1].value), 0);
}

// ============================================================
//  TIMES directive  (DataDirective::count set to repeat value)
// ============================================================

TEST(ParserTimes, ParsesTimesDbZero) {
  // times 10 db 0\n
  std::vector<Token> toks = {make_ident("times"),
                             make_number(10),
                             make_ident("db"),
                             make_number(0),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  ASSERT_EQ(prog.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<DataDirective>(prog[0]));
  const DataDirective &d = std::get<DataDirective>(prog[0]);
  EXPECT_EQ(d.kind, DataDirectiveKind::DB);
  EXPECT_EQ(d.count, 10);
  ASSERT_EQ(d.values.size(), 1u);
  EXPECT_EQ(std::get<int64_t>(d.values[0].value), 0);
}

TEST(ParserTimes, ParsesTimesDwValue) {
  // times 4 dw 0xFF\n
  std::vector<Token> toks = {make_ident("times"),
                             make_number(4),
                             make_ident("dw"),
                             make_number(0xFF),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  const DataDirective &d = std::get<DataDirective>(prog[0]);
  EXPECT_EQ(d.kind, DataDirectiveKind::DW);
  EXPECT_EQ(d.count, 4);
  EXPECT_EQ(std::get<int64_t>(d.values[0].value), 0xFF);
}

// Without 'times', count defaults to 1
TEST(ParserTimes, DefaultCountIsOne) {
  std::vector<Token> toks = {make_ident("db"), make_number(0),
                             make_simple(TokenType::NEWLINE), make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  EXPECT_EQ(std::get<DataDirective>(prog[0]).count, 1);
}

// ============================================================
//  RES* directives  (values must be empty; count = operand)
// ============================================================

TEST(ParserRes, ParsesResbWithCount) {
  std::vector<Token> toks = {make_ident("resb"), make_number(64),
                             make_simple(TokenType::NEWLINE), make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  ASSERT_EQ(prog.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<DataDirective>(prog[0]));
  const DataDirective &d = std::get<DataDirective>(prog[0]);
  EXPECT_EQ(d.kind, DataDirectiveKind::RESB);
  EXPECT_EQ(d.count, 64);
  EXPECT_TRUE(d.values.empty());
}

TEST(ParserRes, ParsesReswWithCount) {
  std::vector<Token> toks = {make_ident("resw"), make_number(8),
                             make_simple(TokenType::NEWLINE), make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  const DataDirective &d = std::get<DataDirective>(prog[0]);
  EXPECT_EQ(d.kind, DataDirectiveKind::RESW);
  EXPECT_EQ(d.count, 8);
  EXPECT_TRUE(d.values.empty());
}

TEST(ParserRes, ParsesResdWithCount) {
  std::vector<Token> toks = {make_ident("resd"), make_number(2),
                             make_simple(TokenType::NEWLINE), make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  const DataDirective &d = std::get<DataDirective>(prog[0]);
  EXPECT_EQ(d.kind, DataDirectiveKind::RESD);
  EXPECT_EQ(d.count, 2);
  EXPECT_TRUE(d.values.empty());
}

TEST(ParserRes, ParsesResqWithCount) {
  std::vector<Token> toks = {make_ident("resq"), make_number(1),
                             make_simple(TokenType::NEWLINE), make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  const DataDirective &d = std::get<DataDirective>(prog[0]);
  EXPECT_EQ(d.kind, DataDirectiveKind::RESQ);
  EXPECT_EQ(d.count, 1);
  EXPECT_TRUE(d.values.empty());
}

// ============================================================
//  ALIGN directive  (DataDirectiveKind::ALIGN, count = alignment)
// ============================================================

TEST(ParserAlign, ParsesAlignWithPowerOfTwo) {
  // align 16\n
  std::vector<Token> toks = {make_ident("align"), make_number(16),
                             make_simple(TokenType::NEWLINE), make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  ASSERT_EQ(prog.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<DataDirective>(prog[0]));
  const DataDirective &d = std::get<DataDirective>(prog[0]);
  EXPECT_EQ(d.kind, DataDirectiveKind::ALIGN);
  EXPECT_EQ(d.count, 16);
}

// align 
TEST(ParserAlign, AlignWithExtraArgumentProducesError) {
  std::vector<Token> toks = {make_ident("align"),
                             make_number(4),
                             make_simple(TokenType::COMMA),
                             make_number(0x90),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};
  Parser p(toks);
  p.parse();
  EXPECT_TRUE(p.hasErrors());
}

// ============================================================
//  Integration tests
// ============================================================

TEST(ParserIntegration, FullMinimalProgram) {
  // global _start
  // section .text
  // _start:
  //   mov rax, 1
  //   mov rdi, 0
  //   ret
  std::vector<Token> toks = {make_ident("global"),
                             make_ident("_start"),
                             make_simple(TokenType::NEWLINE),
                             make_ident("section"),
                             make_simple(TokenType::DOT),
                             make_ident("text"),
                             make_simple(TokenType::NEWLINE),
                             make_ident("_start"),
                             make_simple(TokenType::COLON),
                             make_simple(TokenType::NEWLINE),
                             make_ident("mov"),
                             make_ident("rax"),
                             make_simple(TokenType::COMMA),
                             make_number(1),
                             make_simple(TokenType::NEWLINE),
                             make_ident("mov"),
                             make_ident("rdi"),
                             make_simple(TokenType::COMMA),
                             make_number(0),
                             make_simple(TokenType::NEWLINE),
                             make_ident("ret"),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  ASSERT_EQ(prog.size(), 6u);
  EXPECT_TRUE(std::holds_alternative<GlobalDirective>(prog[0]));
  EXPECT_EQ(std::get<GlobalDirective>(prog[0]).name, "_start");
  ASSERT_TRUE(std::holds_alternative<SectionDirective>(prog[1]));
  EXPECT_EQ(std::get<SectionDirective>(prog[1]).type,
            SectionDirectiveType::TEXT);
  ASSERT_TRUE(std::holds_alternative<Label>(prog[2]));
  EXPECT_EQ(std::get<Label>(prog[2]).name, "_start");
  EXPECT_TRUE(std::holds_alternative<Instruction>(prog[3]));
  EXPECT_TRUE(std::holds_alternative<Instruction>(prog[4]));
  EXPECT_TRUE(std::holds_alternative<Instruction>(prog[5]));
}

TEST(ParserIntegration, DataSectionWithNullTerminatedString) {
  // section .data
  // msg: db "hello", 0
  std::vector<Token> toks = {make_ident("section"),
                             make_simple(TokenType::DOT),
                             make_ident("data"),
                             make_simple(TokenType::NEWLINE),
                             make_ident("msg"),
                             make_simple(TokenType::COLON),
                             make_ident("db"),
                             make_string("hello"),
                             make_simple(TokenType::COMMA),
                             make_number(0),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  ASSERT_EQ(prog.size(), 3u);
  EXPECT_EQ(std::get<SectionDirective>(prog[0]).type,
            SectionDirectiveType::DATA);
  EXPECT_EQ(std::get<Label>(prog[1]).name, "msg");
  const DataDirective &d = std::get<DataDirective>(prog[2]);
  EXPECT_EQ(d.kind, DataDirectiveKind::DB);
  ASSERT_EQ(d.values.size(), 2u);
  EXPECT_TRUE(std::holds_alternative<std::string>(d.values[0].value));
  EXPECT_EQ(std::get<int64_t>(d.values[1].value), 0);
}

TEST(ParserIntegration, BssSectionWithResb) {
  // section .bss
  // buf: resb 128
  std::vector<Token> toks = {make_ident("section"),
                             make_simple(TokenType::DOT),
                             make_ident("bss"),
                             make_simple(TokenType::NEWLINE),
                             make_ident("buf"),
                             make_simple(TokenType::COLON),
                             make_ident("resb"),
                             make_number(128),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  ASSERT_EQ(prog.size(), 3u);
  EXPECT_EQ(std::get<SectionDirective>(prog[0]).type,
            SectionDirectiveType::BSS);
  EXPECT_EQ(std::get<Label>(prog[1]).name, "buf");
  const DataDirective &d = std::get<DataDirective>(prog[2]);
  EXPECT_EQ(d.kind, DataDirectiveKind::RESB);
  EXPECT_EQ(d.count, 128);
  EXPECT_TRUE(d.values.empty());
}

TEST(ParserIntegration, EquThenInstruction) {
  // SIZE equ 256
  // mov rax, SIZE
  std::vector<Token> toks = {make_ident("SIZE"),
                             make_ident("equ"),
                             make_number(256),
                             make_simple(TokenType::NEWLINE),
                             make_ident("mov"),
                             make_ident("rax"),
                             make_simple(TokenType::COMMA),
                             make_ident("SIZE"),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  ASSERT_EQ(prog.size(), 2u);
  ASSERT_TRUE(std::holds_alternative<EquDirective>(prog[0]));
  EXPECT_EQ(std::get<EquDirective>(prog[0]).value, 256);
  ASSERT_TRUE(std::holds_alternative<Instruction>(prog[1]));
  // SIZE is not a register — must be a label/symbol operand, never a register
  const Operand &src = std::get<Instruction>(prog[1]).operands[1];
  EXPECT_FALSE(src.isReg());
}

// ============================================================
//  Error-recovery tests
// ============================================================

// A bare comma at the start of a line is invalid
TEST(ParserErrors, BareCommaProducesError) {
  std::vector<Token> toks = {make_simple(TokenType::COMMA),
                             make_simple(TokenType::NEWLINE), make_eof()};

  Parser p(toks);
  p.parse();
  EXPECT_TRUE(p.hasErrors());
}

// Missing closing bracket — must report error and not crash
TEST(ParserErrors, UnclosedMemoryOperandProducesError) {
  // mov rax, [rbx\n   (missing ']')
  std::vector<Token> toks = {make_ident("mov"),
                             make_ident("rax"),
                             make_simple(TokenType::COMMA),
                             make_simple(TokenType::LBRACKET),
                             make_ident("rbx"),
                             make_simple(TokenType::NEWLINE),
                             make_eof()};

  Parser p(toks);
  p.parse();
  EXPECT_TRUE(p.hasErrors());
}

// Extra blank lines between statements must be silently ignored
TEST(ParserErrors, TrailingNewlinesAreHarmless) {
  std::vector<Token> toks = {make_ident("ret"), make_simple(TokenType::NEWLINE),
                             make_simple(TokenType::NEWLINE),
                             make_simple(TokenType::NEWLINE), make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_FALSE(p.hasErrors()) << formatErrors(p);
  ASSERT_EQ(prog.size(), 1u);
}

// An error on one line must not swallow the next valid statement
TEST(ParserErrors, RecoveryAfterBadLine) {
  // ,\n
  // ret\n
  std::vector<Token> toks = {make_simple(TokenType::COMMA),
                             make_simple(TokenType::NEWLINE), make_ident("ret"),
                             make_simple(TokenType::NEWLINE), make_eof()};

  Parser p(toks);
  FlatProgram prog = p.parse();
  EXPECT_TRUE(p.hasErrors());
  // Despite the error, 'ret' should still appear in the output
  ASSERT_GE(prog.size(), 1u);
  EXPECT_TRUE(std::holds_alternative<Instruction>(prog.back()));
}

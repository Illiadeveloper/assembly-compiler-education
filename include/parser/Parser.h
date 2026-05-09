/// @file Parser.h
/// @brief Parsing Tokens from Lexer to FlatProgram
///
/// Converts a sequence of tokens (produced by the Lexer) into a flat
/// intermediate representation (FlatProgram).
///
/// The parser performs a single-pass over the token stream, maintaining
/// an internal cursor and incrementally building AST nodes.
///
/// Design notes:
/// - Error-tolerant: collects errors and attempts recovery by skipping lines
/// - Performs only syntactic analysis; semantic validation is deferred
///   to later compilation stages
///
#pragma once
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "ast/Instruction.h"
#include "ast/Statement.h"
#include "common/CompilerError.h"
#include "common/Registers.h"
#include "common/Token.h"
#include "parser/MemoryExpression.h"

/// @brief Assembly parser converting tokens into FlatProgram
///
/// Maintains a cursor over the token stream and builds AST nodes incrementally
/// The output is a flat intermediate representation (FlatProgram), where
/// directives and instructions are stored sequentially
///
/// Parsing strategy:
/// - Top-level constructs are parsed line-by-line
/// - Each line may represent a directive, label, or instruction
/// - Operands are parsed individually and may include registers,
///   immediates, labels, or memory expressions
///
/// Error handling:
/// - Errors are accumulated instead of thrown immediately
/// - On failure, the parser attempts recovery by skipping to the next line
///
/// Example Code:
///   section .text:
///     mov rax, 32
///
/// Example Lexer Result:
///   [Token(TokenType::IDENTIFIER, {span}, "section"),
///   Token(TokenType::DOT, {span}, "."),
///   Token(TokenType::IDENTIFIER, {span}, "text"),
///   Token(TokenType::COLON, {span}, ":"),
///   Token(TokenType::NEWLINE, {span}, "\n"),
///   Token(TokenType::IDENTIFIER, {span}, "mov"),
///   Token(TokenType::IDENTIFIER, {span}, "rax"),
///   Token(TokenType::NUMBER, {span}, 32)]
///
/// Example Parser Result:
///   FlatProgram[
///     SectionDirective(SectionDirectiveType::TEXT, {span}),
///     Instruction(Opcode::MOV, [
///       Operand(OperandKind::REG, OperandSize::B32, Register::RAX, {span}),
///       Operand(OperandKind::IMM, OperandSize::ANY, 32, {span})
///     ], {span})
///   ]
///
class Parser {
private:
  std::vector<Token> tokens;
  size_t currentToken = 0;

  /// =========================
  /// Errors
  /// =========================
  std::vector<CompilerError> errors;
  void addError(const SourceSpan &span, const std::string &msg);

  /// =========================
  /// Token navigation
  /// =========================

  const Token &peek(size_t offset = 0) const;
  const Token &advance();
  void skipToNextLine();
  bool endOfFile() const;

  /// =========================
  /// Matching & helpers
  /// =========================

  bool match(TokenType type, size_t offset = 0) const;
  bool tokenIsDataDirective(const Token &token) const;
  std::optional<int64_t> parseSignedInteger();

  /// =========================
  /// Classification
  /// =========================

  bool isGlobalDirective() const;
  bool isDataDirective() const;
  bool isSectionDirective() const;
  bool isEquDirection() const;

  bool isLabelDeclaration() const;
  bool isInstruction() const;
  bool tokenIsRegister(const Token &token) const;

  bool isRegister() const;
  bool isMemoryOperand() const;
  bool isImmediate() const;
  bool isLabelOperand() const;

  /// =========================
  /// Memory operand parsing
  /// =========================

  /// Parses arithmetic expressions used in memory operands
  /// (e.g., [rax + rbx*2 + 8])
  /// Parses an arithmetic expression used in memory operands.
  ///
  /// Implements a precedence-based (Pratt-style) parser to handle
  /// operators like +, -, * and parentheses.
  ///
  /// Example:
  ///   [rax + rbx*2 + 8]
  ///
  /// @param rbp Right-binding power (operator precedence control)
  /// @return Root node of the constructed expression AST
  std::unique_ptr<MemoryExpression::Node> parseExpression(int rbp = 0);

  /// Flattens an expression AST into a linear representation.
  ///
  /// Traverses the expression tree and collects terms with their
  /// corresponding signs into the output vector.
  ///
  /// Example:
  ///   rax + rbx*2 - 8  →  [(+1, rax), (+1, rbx*2), (-1, 8)]
  ///
  /// @param node Root of the expression AST
  /// @param sign Current accumulated sign (+1 or -1)
  /// @param out Output vector of (sign, node) pairs
  void flattenAST(MemoryExpression::Node *node, int sign,
                  std::vector<std::pair<int, MemoryExpression::Node *>> &out);

  /// =========================
  /// Parsing primitives
  /// =========================

  /// Parses a memory operand expression.
  ///
  /// Handles bracketed expressions and converts them into a structured
  /// MemoryOperand representation.
  ///
  /// Example:
  ///   [rax + rbx*2 + 8]
  ///
  /// @return Parsed MemoryOperand or std::nullopt on failure
  std::optional<MemoryOperand> parseMemoryOperand();
  Register parseRegister();
  Label parseLabelOperand();
  Label parseLabelDeclaration();

  /// =========================
  /// Directives
  /// =========================

  std::optional<GlobalDirective> parseGlobalDirective();
  std::optional<DataDirective> parseDataDirective();
  std::optional<SectionDirective> parseSectionDirective();
  std::optional<EquDirective> parseEquDirective();

  /// =========================
  /// High-level constructs
  /// =========================

  std::optional<Operand> parseOperand();
  std::optional<Instruction> parseInstruction();

public:
  explicit Parser(std::vector<Token> tokens);
  FlatProgram parse();

  const std::vector<CompilerError> &getErrors() const noexcept;
  bool hasErrors() const noexcept;
};

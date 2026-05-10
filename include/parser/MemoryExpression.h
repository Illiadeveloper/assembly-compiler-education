/// @file MemoryExpression.h
/// @brief AST used to represent parsed memory addressing expressions
#pragma once
#include "common/Token.h"
#include <memory>

/// @brief Represents parsed expressions used in memory addressing
///
/// @details
/// This structure is used by the parser to represent memory operands,
/// such as:
///
///   [eax]
///   [ebx + 4]
///   [rax + rcx * 2 - 8]
///
/// The expression is converted into an Abstract Syntax Tree (AST),
/// which simplifies further processing and encoding into ModRM/SIB
/// addressing modes.
struct MemoryExpression {

  /// @brief Node of the AST representing a memory expression
  struct Node {
    /// @brief Type of AST node
    enum Kind {
      NUMBER,     ///< constant value (4, etc.)
      REGISTER,   ///< CPU registers(rax, etc.)
      IDENTIFIER, ///< symbol or label
      OPERAND,    ///< binary operand ('-', '+', '*', etc.)
      UNARY       ///< unary operand(-x, etc.)
    } kind;

    /// @brief Constant value (valid if kind == NUMBER)
    int64_t value;

    /// @brief Register name or identifier
    /// Valid if kind == REGISTER or IDENTIFIER
    std::string name;

    /// @brief Operator symbol (e.g., '+', '-', '*')
    /// Valid if kind == OPERAND or UNARY
    char operand;

    /// @brief Left and right children of the AST node
    std::unique_ptr<Node> left, right;
  };

  /// @brief Returns binding power (operator precedence)
  ///
  /// @details
  /// Used in Pratt parsing to control operator precedence:
  ///
  /// - '*' has higher precedence than '+' and '-'
  /// - Higher value means higher precedence
  ///
  /// Example:
  ///   a + b * c  → '*' is evaluated before '+'
  static int bindingPower(TokenType t) {
    switch (t) {
    case TokenType::STAR:
      return 60;
    case TokenType::MINUS:
    case TokenType::PLUS:
      return 40;
    default:
      return 0;
    }
  }
};

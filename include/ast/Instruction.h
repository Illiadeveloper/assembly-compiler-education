/// @file Instruction.h
/// @brief Instruction AST structures

#pragma once
#include "common/Registers.h"
#include "common/Token.h"
#include "opcodes/Opcode.h"
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

/// Signed symbolic term used in memory expressions
struct SymbolTerm {
  int sign;         ///< +1 or -1
  std::string name; ///< Symbol name
};

/// Represents x86 memory operand.
///
/// Examples:
///   [rax]
///   [rax + 8]
///   [rax + rbx*4]
///   [symbol + rax*2 - 16]
///
/// Layout:
///   base + index * scale + displacement + symbols
struct MemoryOperand {
  std::optional<Register> base;  /// rax
  std::optional<Register> index; /// rbx
  int scale = 1;                 /// 1,2,4,8

  int64_t displacement = 0; /// +8, -4, etc
  OperandSize size = OperandSize::ANY;
  std::vector<SymbolTerm> symbols; ///< Symbolic terms in the expression
};

/// Size prefix mapping ('byte', 'word', ...)
inline const std::unordered_map<std::string, OperandSize> SizePrefixMap = {
    {"byte", OperandSize::B8},
    {"word", OperandSize::B16},
    {"dword", OperandSize::B32},
    {"qword", OperandSize::B64}};

inline constexpr std::string_view PTR_KEYWORD = "ptr";

/// Instruction operand
struct Operand {
  OperandKind kind;
  OperandSize size; ///< Operand size (ANY if not specified or label)

  std::variant<Register,      ///< if REG
               int64_t,       ///< if IMM
               MemoryOperand, ///< if MEM
               std::string    ///< if LABEL (labels)
               >
      value;
  SourceSpan span;

  // convenience helpers
  bool isReg() const noexcept { return kind == OperandKind::REG; }
  bool isImm() const noexcept { return kind == OperandKind::IMM; }
  bool isMem() const noexcept { return kind == OperandKind::MEM; }
  bool isLabel() const noexcept { return kind == OperandKind::LABEL; }

  Register getReg() const { return std::get<Register>(value); }
  int64_t getImm() const { return std::get<int64_t>(value); }
  const MemoryOperand &getMem() const { return std::get<MemoryOperand>(value); }
  const std::string &getLabel() const { return std::get<std::string>(value); }

  static Operand makeReg(Register r, OperandSize s, SourceSpan sp) {
    Operand o;
    o.kind = OperandKind::REG;
    o.size = s;
    o.span = sp;
    o.value = r;
    return o;
  }
  static Operand makeImm(int64_t v, OperandSize s, SourceSpan sp) {
    Operand o;
    o.kind = OperandKind::IMM;
    o.size = s;
    o.span = sp;
    o.value = v;
    return o;
  }
  static Operand makeMem(MemoryOperand m, OperandSize s, SourceSpan sp) {
    Operand o;
    o.kind = OperandKind::MEM;
    o.size = s;
    o.span = sp;
    o.value = std::move(m);
    return o;
  }
  static Operand makeLabel(std::string name, SourceSpan sp) {
    Operand o;
    o.kind = OperandKind::LABEL;
    o.size = OperandSize::ANY;
    o.span = sp;
    o.value = std::move(name);
    return o;
  }
};

/// Assembly instruction node.
///
/// Example:
///   mov rax, rbx
///   add rax, 5
struct Instruction {
  Opcode opcode;
  std::vector<Operand> operands;
  SourceSpan span;
};

/// @file OperandTypes.h
/// @brief Definitions of operand size and operand kind used in instructions
#pragma once
#include <cstdint>

/// @brief Specifies the size of an operand in bits
/// Example: AL -> B8, AX -> B16, EAX -> B32, RAX -> B64
enum class OperandSize : uint8_t {
  ANY = 0,
  B8 = 8,
  B16 = 16,
  B32 = 32,
  B64 = 64
};

/// @brief Possible operand kinds used in instructions
enum class OperandKind {
  REG,  ///< rax, rbx, etc
  IMM,  ///< 8, 16, -25, etc
  MEM,  ///< [symbol + rax*2 - 16], etc
  LABEL ///< label1, symbol, etc
};

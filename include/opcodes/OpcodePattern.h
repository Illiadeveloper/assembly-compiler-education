/// @file OpcodePattern.h
/// @brief Defines structures describing how instructions are encoded into
/// machine code
#pragma once
#include <cstdint>
#include <optional>
#include <vector>

#include "Opcode.h"
#include "common/OperandTypes.h"
#include "common/Registers.h"

// enum RegClassMask : uint8_t {
//   RC_NONE  = 0,
//   RC_GPR8  = 1 << 0,
//   RC_GPR16 = 1 << 1,
//   RC_GPR32 = 1 << 2,
//   RC_GPR64 = 1 << 3,
//
//   RC_GPR_ANY = RC_GPR8 | RC_GPR16 | RC_GPR32 | RC_GPR64
// };
//
// inline RegClassMask operator|(RegClassMask a, RegClassMask b) {
//   return static_cast<RegClassMask>(
//     static_cast<uint8_t>(a) | static_cast<uint8_t>(b)
//   );
// }

/// @brief Bitmask representing allowed operand kinds
enum OperandKindMask : uint8_t {
  OK_NONE = 0,
  OK_REG = 1 << 0,   ///< Register Operand
  OK_MEM = 1 << 1,   ///< Memory Operand
  OK_IMM = 1 << 2,   ///< Immediate value
  OK_LABEL = 1 << 3  ///< Label (for jumps, call, memoty access...)
};

/// @brief Bitwise OR operator for OperandKindMask.
inline OperandKindMask operator|(OperandKindMask a,
                                 OperandKindMask b) {  // for OK_REG | OK_MEM
  return static_cast<OperandKindMask>(static_cast<uint8_t>(a) |
                                      static_cast<uint8_t>(b));
}

/// @brief Describes constraints for a single operand in an opcode pattern
///
/// @details
/// Defines what types of operands are allowed, their size restrictions,
/// and optionally enforces a specific register
struct OperandConstraint {
  OperandKindMask allowed =
      OK_NONE;  ///< Allowed operand kinds (REG, MEM, IMM, etc.)
  OperandSize size =
      OperandSize::ANY;  ///< Required operand size (e.g., B8, B32, ANY)
  std::optional<Register>
      exactReg;  ///< Specific register requirement (e.g., AL in ADD AL, imm8)
};

/// @brief Specifies additional encoding required after base opcode bytes.
enum class ExtraEncoding {
  NONE,         ///< No extra encoding
  IMM8,         ///< 8-bit immediate
  IMM8_SIGNED,  ///< 8-bit signed immediate
  IMM16,        ///< 16-bit immediate
  IMM32,        ///< 32-bit immediate
  IMM64,        ///< 64-bit immediate
  REL8,         ///< 8-bit relative offset
  REL32         ///< 32-bit relative offset
};
/// ---------------------------------------------------------------------------
/// Encoding reference (for documentation purposes)
/// ---------------------------------------------------------------------------
///
/// ModRM byte layout:
///  7 6 | 5 4 3 | 2 1 0
/// +----+-------+------+
/// |MOD |  REG  |  RM  |
/// +----+-------+------+
///
/// MOD  - addressing mode (register or memory)
/// REG  - register operand or opcode extension (/0, /1, ...)
/// RM   - register or memory operand
///
/// SIB byte layout:
///  7 6 | 5 4 3 | 2 1 0
/// +----+-------+------+
/// |SCAL| INDEX | BASE |
/// +----+-------+------+
///
/// SCALE - scaling factor (×1, ×2, ×4, ×8)
/// INDEX - index register (or 100 = none)
/// BASE  - base register
///
//  7 6 | 5 4 3 | 2 1 0
// +----+-------+------+
// |SCAL| INDEX | BASE |
// +----+-------+------+
// SCAL - scale (x1, x2, x4, x8)
// INDEX - register or 100(mean no registers)
// BASe - base register
// SIB - using in memory operand

/// @brief Describes full encoding pattern for an instruction opcode
///
/// @details
/// This structure is used by the assembler to:
/// - match instruction operands against valid patterns
/// - generate corresponding machine code
///
/// It includes:
/// - opcode identifier
/// - operand constraints
/// - base opcode bytes
/// - ModRM encoding rules
/// - optional register encoding inside opcode
/// - extra encoding (immediates, relative offsets)
/// - operand size grouping rules
struct OpcodePattern {
  Opcode opcode;  ///< Instruction opcode (e.g., MOV, ADD, LEA)

  /// @brief Constraints for each operand in order
  std::vector<OperandConstraint> operands;

  /// @brief Base opcode bytes (without ModRM / immediates)
  /// Example: {0xB8}, {0x01}
  std::vector<uint8_t> baseBytes;

  /// @brief Indicates whether a ModRM byte is required.
  bool hasModRM = false;

  /// @brief Fixed REG field value in ModRM (opcode extension like /0, /1,
  /// etc.)
  std::optional<uint8_t> modrm_reg;

  /// @brief If true, register index is encoded directly in opcode (e.g., B8 +
  /// r)
  bool regInOpcode = false;

  /// @brief Specifies additional encoding appended after base bytes
  ExtraEncoding extra = ExtraEncoding::NONE;

  /// @brief Groups of operands that must share the same size.
  ///
  /// @details
  /// Each value represents a group ID. Operands with the same group ID
  /// must have identical sizes.
  ///
  /// Example:
  ///   {0, 0} → both operands must have the same size
  ///   {0, 1} → operands can have different sizes
  ///
  /// Value -1 means no size restriction.
  std::vector<uint8_t> operandSizeGroup;
};

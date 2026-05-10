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

/// @brief Bitmask representing allowed operand kinds
enum OperandKindMask : uint8_t {
  OK_NONE = 0,
  OK_REG = 1 << 0,   ///< Register operand
  OK_MEM = 1 << 1,   ///< Memory operand
  OK_IMM = 1 << 2,   ///< Immediate value
  OK_LABEL = 1 << 3  ///< Label (for jumps, calls, memory access)
};

/// @brief Bitwise OR operator for OperandKindMask
inline OperandKindMask operator|(OperandKindMask a, OperandKindMask b) {
  return static_cast<OperandKindMask>(static_cast<uint8_t>(a) |
                                      static_cast<uint8_t>(b));
}

/// @brief Constraints for a single operand in an opcode pattern
///
/// Defines what types of operands are allowed, their size restrictions,
/// and optionally enforces a specific register.
struct OperandConstraint {
  OperandKindMask allowed =
      OK_NONE;  ///< Allowed operand kinds (REG, MEM, IMM, LABEL)
  OperandSize size =
      OperandSize::ANY;  ///< Required operand size (B8, B16, B32, B64, ANY)
  std::optional<Register>
      exactReg;  ///< Exact register requirement (e.g. AL for `ADD AL, imm8`)
};

/// @brief Additional encoding appended after base opcode bytes
enum class ExtraEncoding {
  NONE,         ///< No extra encoding
  IMM8,         ///< 8-bit immediate
  IMM8_SIGNED,  ///< 8-bit signed immediate
  IMM16,        ///< 16-bit immediate
  IMM32,        ///< 32-bit immediate
  IMM64,        ///< 64-bit immediate
  REL8,         ///< 8-bit relative offset (short jump)
  REL32         ///< 32-bit relative offset (near jump)
};

/// @brief Full encoding pattern for a single instruction form
///
/// Used by `InstructionEncoder` to match instruction operands and
/// generate corresponding x86-64 machine code bytes.
///
/// @par ModRM byte layout
/// @code
///  7  6 | 5  4  3 | 2  1  0
/// +-----+---------+---------+
/// | MOD |   REG   |   RM    |
/// +-----+---------+---------+
///
/// MOD — addressing mode:
///   11 = register direct
///   10 = memory + disp32
///   01 = memory + disp8
///   00 = memory (no displacement)
///
/// REG — register operand or opcode extension (/0, /1, ...)
/// RM  — register or memory operand
/// @endcode
///
/// @par SIB byte layout
/// @code
///  7  6 | 5  4  3 | 2  1  0
/// +-----+---------+---------+
/// | SS  |  INDEX  |  BASE   |
/// +-----+---------+---------+
///
/// SS    — scale factor: 00=×1, 01=×2, 10=×4, 11=×8
/// INDEX — index register (100 = no index)
/// BASE  — base register
/// @endcode
///
/// @par Example patterns
/// @code
/// // ADD r/m8, r8  → opcode 0x00, ModRM required
/// OpcodePattern {
///   baseBytes = { 0x00 },
///   hasModRM  = true,
///   extra     = NONE
/// }
///
/// // MOV r64, imm64  → REX.W + B8+r + 8 bytes immediate
/// OpcodePattern {
///   baseBytes   = { 0xB8 },
///   regInOpcode = true,
///   extra       = IMM64
/// }
/// @endcode
struct OpcodePattern {
  Opcode opcode;  ///< Instruction opcode (e.g. MOV, ADD, LEA)

  /// @brief Constraints for each operand in order (left to right)
  std::vector<OperandConstraint> operands;

  /// @brief Base opcode bytes (without ModRM or immediates)
  /// @example `{0x01}` for ADD, `{0x0F, 0x84}` for JE
  std::vector<uint8_t> baseBytes;

  /// @brief Whether a ModRM byte must be emitted
  bool hasModRM = false;

  /// @brief Fixed REG field in ModRM — opcode extension (/0, /1, etc.)
  /// @details Set when the REG field is not a register but an opcode extension.
  /// For example, `ADD r/m, imm` uses `/0` (modrm_reg = 0).
  /// If nullopt, REG field is taken from the register operand.
  std::optional<uint8_t> modrm_reg;

  /// @brief Whether the register index is encoded inside the opcode byte
  /// @details Used for short forms like `MOV r64, imm64` (B8+rd)
  /// and `PUSH r64` (50+rd).
  bool regInOpcode = false;

  /// @brief Additional encoding emitted after base opcode bytes
  ExtraEncoding extra = ExtraEncoding::NONE;

  /// @brief Operand size groups — operands with the same group ID must match in
  /// size
  ///
  /// @details
  /// Example:
  /// @code
  /// {0, 0}  → both operands must have the same size
  /// {0, 1}  → operands can differ in size
  /// @endcode
  /// Value 0xFF means no size restriction for that operand.
  std::vector<uint8_t> operandSizeGroup;
};

/// @file OpcodeFactories.h
/// @brief Factories for generating instruction opcode patterns
/// @details
/// This file provides helper structs (factories) for building OpcodePattern
/// instances for different instructions (ADD, MOV, SUB, etc.).
/// Each factory struct contains static functions that generate specific
/// instruction forms (e.g., register-to-register, register-to-memory,
/// accumulator with immediate, etc.).
/// Using these factories makes building the global OpcodeTable readable
/// and maintainable.
#pragma once
#include "OpcodePattern.h"

// =================================================
// Data movement
// =================================================


/// @brief Factory for MOV instruction patterns
///
/// @details Copies data from source to destination. Source and destination
/// must be the same size. MOV never reads or writes memory implicitly —
/// all operands are explicit.
///
/// Encoding reference:
///   MOV r/m8,  r8    → 0x88  /r
///   MOV r/m16, r16   → 0x89  /r  (with 0x66 prefix)
///   MOV r/m32, r32   → 0x89  /r
///   MOV r/m64, r64   → REX.W 0x89  /r
///   MOV r8,    r/m8  → 0x8A  /r
///   MOV r16,   r/m16 → 0x8B  /r  (with 0x66 prefix)
///   MOV r32,   r/m32 → 0x8B  /r
///   MOV r64,   r/m64 → REX.W 0x8B  /r
///   MOV r8,    imm8  → 0xB0+r  ib       (short B0+r form, no ModRM)
///   MOV r16,   imm16 → 0xB8+r  iw
///   MOV r32,   imm32 → 0xB8+r  id
///   MOV r64,   imm64 → REX.W 0xB8+r  iq
///   MOV r/m8,  imm8  → 0xC6  /0  ib
///   MOV r/m16, imm16 → 0xC7  /0  iw
///   MOV r/m32, imm32 → 0xC7  /0  id
///   MOV r/m64, imm32 → REX.W 0xC7  /0  id  (sign-extended to 64 bits)
struct MovPatterns {
  /// @brief MOV r/m, r — copy register into register or memory
  static OpcodePattern rm_r(OperandSize sz, uint8_t base);
 
  /// @brief MOV r, r/m — copy register or memory into register
  static OpcodePattern r_rm(OperandSize sz, uint8_t base);
 
  /// @brief MOV r, imm — load immediate directly into register (B8+r form)
  /// @note Uses regInOpcode encoding: no ModRM byte, register index is
  ///       added to the base opcode byte (e.g., MOV RAX → 0x48 0xB8)
  static OpcodePattern r_imm(OperandSize sz, uint8_t base);
 
  /// @brief MOV r/m, imm — store immediate into register or memory (C6/C7 form)
  /// @note Uses ModRM /0. For 64-bit destinations, the immediate is
  ///       sign-extended from 32 bits.
  static OpcodePattern rm_imm(OperandSize sz, uint8_t base);
};

/// @brief Factory for LEA instruction patterns
///
/// @details Computes the effective address of the memory operand and stores
/// it in the destination register. Unlike MOV, LEA never accesses memory —
/// it only calculates the address
///
/// Encoding reference:
///   LEA r16, m → 0x8D  /r  (with 0x66 prefix)
///   LEA r32, m → 0x8D  /r
///   LEA r64, m → REX.W 0x8D  /r
///
/// @note LEA has no 8-bit form. The memory operand size is irrelevant —
///       only the destination register size matters for the result.
struct LeaPatterns {
  /// @brief LEA r, m — compute and load effective address into register
  static OpcodePattern r_m(OperandSize sz, uint8_t base);
};

/// @brief Factory for PUSH instruction patterns
///
/// @details Decrements RSP by the operand size, then stores the operand
/// at the new top of stack. In 64-bit mode, the default operand size is
/// 64 bits (32-bit push is not encodable without a prefix).
///
/// Encoding reference:
///   PUSH r16   → 0x50+r        (with 0x66 prefix)
///   PUSH r64   → 0x50+r
///   PUSH imm8  → 0x6A  ib      (sign-extended to stack operand size)
///   PUSH imm32 → 0x68  id      (sign-extended to 64 bits)
///   PUSH r/m16 → 0xFF  /6      (with 0x66 prefix)
///   PUSH r/m64 → 0xFF  /6
///
/// @note In 64-bit mode, PUSH imm16 is rarely used and is omitted here.
///       PUSH imm8 sign-extends the byte to fill the full stack slot.
struct PushPatterns {
  /// @brief PUSH r — push register onto stack (50+r form, no ModRM)
  static OpcodePattern r(OperandSize sz, uint8_t base);
 
  /// @brief PUSH imm8 — push sign-extended 8-bit immediate (0x6A form)
  static OpcodePattern imm8(uint8_t base);
 
  /// @brief PUSH imm32 — push sign-extended 32-bit immediate (0x68 form)
  static OpcodePattern imm32(uint8_t base);
 
  /// @brief PUSH r/m — push register or memory operand (0xFF /6 form)
  static OpcodePattern rm(OperandSize sz, uint8_t base);
};


/// @brief Factory for POP instruction patterns
///
/// @details Loads the value at the current top of stack into the destination,
/// then increments RSP by the operand size. In 64-bit mode, the default
/// operand size is 64 bits.
///
/// Encoding reference:
///   POP r64   → 0x58+r         (short form, no ModRM)
///   POP r16   → 0x58+r         (with 0x66 prefix)
///   POP r/m64 → 0x8F  /0
///   POP r/m16 → 0x8F  /0       (with 0x66 prefix)
struct PopPatterns {
  /// @brief POP r — pop top of stack into register (58+r form, no ModRM)
  static OpcodePattern r(OperandSize sz, uint8_t base);
 
  /// @brief POP r/m — pop top of stack into register or memory (0x8F /0 form)
  static OpcodePattern rm(OperandSize sz, uint8_t base);
};

// =================================================
// Arithmetic
// =================================================

/// @brief Factory for ADD instruction patterns
///
/// @details Adds source to destination and stores the result in destination.
/// Updates flags: OF, SF, ZF, AF, CF, PF.
///
/// Encoding reference:
///   ADD r/m8,  r8    → 0x00  /r
///   ADD r/m16, r16   → 0x01  /r  (with 0x66 prefix)
///   ADD r/m32, r32   → 0x01  /r
///   ADD r/m64, r64   → REX.W 0x01  /r
///   ADD r8,    r/m8  → 0x02  /r
///   ADD r16,   r/m16 → 0x03  /r  (with 0x66 prefix)
///   ADD r32,   r/m32 → 0x03  /r
///   ADD r64,   r/m64 → REX.W 0x03  /r
///   ADD AL,    imm8  → 0x04  ib   (shortform, no ModRM)
///   ADD r/m8,  imm8  → 0x80  /0  ib
///   ADD r/m16, imm16 → 0x81  /0  iw
///   ADD r/m32, imm32 → 0x81  /0  id
///   ADD r/m64, imm32 → REX.W 0x81  /0  id  (sign-extended)
struct AddPatterns {
  /// @brief ADD r/m, r — add register into register or memory
  static OpcodePattern rm_r(OperandSize sz, uint8_t base);
 
  /// @brief ADD r, r/m — add register or memory into register
  static OpcodePattern r_rm(OperandSize sz, uint8_t base);
 
  /// @brief ADD AL, imm8 — accumulator shortform, no ModRM (0x04)
  static OpcodePattern al_imm8(uint8_t base);
 
  /// @brief ADD r/m, imm — add immediate into register or memory
  /// @param modrm  Fixed REG field (/0 for ADD)
  static OpcodePattern rm_imm(OperandSize sz, uint8_t base, uint8_t modrm);
};

/// @brief Factory for SUB instruction patterns
///
/// @details SUB is structurally identical to ADD but uses different opcode
/// bytes and /5 as the ModRM extension for the r/m+imm forms.
///
/// Encoding reference:
///   SUB r/m8,  r8    → 0x28  /r
///   SUB r/m16, r16   → 0x29  /r  (with 0x66 prefix)
///   SUB r/m32, r32   → 0x29  /r
///   SUB r/m64, r64   → REX.W 0x29  /r
///   SUB r8,    r/m8  → 0x2A  /r
///   SUB r16,   r/m16 → 0x2B  /r  (with 0x66 prefix)
///   SUB r32,   r/m32 → 0x2B  /r
///   SUB r64,   r/m64 → REX.W 0x2B  /r
///   SUB AL,    imm8  → 0x2C  ib
///   SUB r/m8,  imm8  → 0x80  /5  ib
///   SUB r/m16, imm16 → 0x81  /5  iw
///   SUB r/m32, imm32 → 0x81  /5  id
///   SUB r/m64, imm32 → REX.W 0x81  /5  id  (sign-extended)
struct SubPatterns {
  /// @brief SUB r/m, r — memory/register minus register
  static OpcodePattern rm_r(OperandSize sz, uint8_t base);

  /// @brief SUB r, r/m — register minus memory/register
  static OpcodePattern r_rm(OperandSize sz, uint8_t base);

  /// @brief SUB AL, imm8 — accumulator shortform (no ModRM)
  static OpcodePattern al_imm8(uint8_t base);

  /// @brief SUB r/m, imm — register/memory minus immediate
  static OpcodePattern rm_imm(OperandSize sz, uint8_t base, uint8_t modrm);
};

/// @brief Factory for IMUL instruction patterns
///
/// @details IMUL has three distinct forms:
///
///   1) One-operand:  IMUL r/m   — implicit AX/DX:AX/... destination
///   2) Two-operand:  IMUL r, r/m
///   3) Three-operand: IMUL r, r/m, imm
///
/// Encoding reference:
///   IMUL r/m8        → 0xF6  /5
///   IMUL r/m16       → 0xF7  /5  (with 0x66 prefix)
///   IMUL r/m32       → 0xF7  /5
///   IMUL r/m64       → REX.W 0xF7  /5
///   IMUL r16, r/m16  → 0x0F 0xAF  /r
///   IMUL r32, r/m32  → 0x0F 0xAF  /r
///   IMUL r64, r/m64  → REX.W 0x0F 0xAF  /r
///   IMUL r16, r/m16, imm8  → 0x6B  /r  ib  (sign-extended imm8)
///   IMUL r32, r/m32, imm8  → 0x6B  /r  ib
///   IMUL r64, r/m64, imm8  → REX.W 0x6B  /r  ib
///   IMUL r16, r/m16, imm16 → 0x69  /r  iw
///   IMUL r32, r/m32, imm32 → 0x69  /r  id
///   IMUL r64, r/m64, imm32 → REX.W 0x69  /r  id  (sign-extended)
struct ImulPatterns {
  /// @brief IMUL r/m — one-operand form (result in rDX:rAX)
  static OpcodePattern rm(OperandSize sz, uint8_t base);

  /// @brief IMUL r, r/m — two-operand form
  static OpcodePattern r_rm(OperandSize sz);

  /// @brief IMUL r, r/m, imm8 — three-operand form with sign-extended imm8
  static OpcodePattern r_rm_imm8(OperandSize sz);

  /// @brief IMUL r, r/m, imm — three-operand form with imm16/imm32
  static OpcodePattern r_rm_imm(OperandSize sz);
};

/// @brief Factory for MUL instruction patterns
///
/// @details MUL is unsigned multiplication, one-operand only.
/// The second operand is always implicit (AL / AX / EAX / RAX).
/// Result is written to AX / DX:AX / EDX:EAX / RDX:RAX.
///
/// Encoding reference:
///   MUL r/m8  → 0xF6  /4
///   MUL r/m16 → 0xF7  /4  (with 0x66 prefix)
///   MUL r/m32 → 0xF7  /4
///   MUL r/m64 → REX.W 0xF7  /4
struct MulPatterns {
  /// @brief MUL r/m — unsigned multiply, implicit accumulator
  static OpcodePattern rm(OperandSize sz, uint8_t base);
};

/// @brief Factory for IDIV instruction patterns
///
/// @details Signed division. One explicit operand (divisor).
/// Dividend is always implicit (AX / DX:AX / EDX:EAX / RDX:RAX).
/// Quotient → AL/AX/EAX/RAX, Remainder → AH/DX/EDX/RDX.
///
/// Encoding reference:
///   IDIV r/m8  → 0xF6  /7
///   IDIV r/m16 → 0xF7  /7  (with 0x66 prefix)
///   IDIV r/m32 → 0xF7  /7
///   IDIV r/m64 → REX.W 0xF7  /7
struct IdivPatterns {
  /// @brief IDIV r/m — signed divide, implicit dividend
  static OpcodePattern rm(OperandSize sz, uint8_t base);
};

/// @brief Factory for DIV instruction patterns
///
/// @details Unsigned division. One explicit operand (divisor).
/// Dividend is always implicit (AX / DX:AX / EDX:EAX / RDX:RAX).
/// Quotient → AL/AX/EAX/RAX, Remainder → AH/DX/EDX/RDX.
///
/// Encoding reference:
///   DIV r/m8  → 0xF6  /6
///   DIV r/m16 → 0xF7  /6  (with 0x66 prefix)
///   DIV r/m32 → 0xF7  /6
///   DIV r/m64 → REX.W 0xF7  /6
struct DivPatterns {
  /// @brief DIV r/m — unsigned divide, implicit dividend
  static OpcodePattern rm(OperandSize sz, uint8_t base);
};

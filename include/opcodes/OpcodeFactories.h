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

  /// @brief LEA r, label — load RIP-relative address of a label (0x8D, mod=00
  /// rm=101)
  /// @note Encodes as LEA r64, [RIP + rel32]. The assembler must emit a
  ///       ModRM byte with mod=00, rm=101 and a 32-bit RIP-relative offset.
  static OpcodePattern r_label(OperandSize sz);
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

/// @brief Factory for INC instruction patterns
///
/// @details Increments the operand by 1. Equivalent to ADD r/m, 1 but
/// produces a shorter encoding. Unlike ADD, INC does NOT modify the CF flag —
/// all other arithmetic flags (OF, SF, ZF, AF, PF) are updated normally
///
/// Encoding reference:
///   INC r/m8  → 0xFE  /0
///   INC r/m16 → 0xFF  /0  (with 0x66 prefix)
///   INC r/m32 → 0xFF  /0
///   INC r/m64 → REX.W 0xFF  /0
struct IncPatterns {
  /// @brief INC r/m — increment register or memory by 1
  static OpcodePattern rm(OperandSize sz, uint8_t base);
};

/// @brief Factory for DEC instruction patterns
///
/// @details Decrements the operand by 1. Equivalent to SUB r/m, 1 but
/// produces a shorter encoding. Like INC, DEC does NOT modify the CF flag —
/// all other arithmetic flags (OF, SF, ZF, AF, PF) are updated normally
///
/// Encoding reference:
///   DEC r/m8  → 0xFE  /1
///   DEC r/m16 → 0xFF  /1  (with 0x66 prefix)
///   DEC r/m32 → 0xFF  /1
///   DEC r/m64 → REX.W 0xFF  /1
struct DecPatterns {
  /// @brief DEC r/m — decrement register or memory by 1
  static OpcodePattern rm(OperandSize sz, uint8_t base);
};

/// @brief Factory for NEG instruction patterns
///
/// @details Computes the two's complement negation of the operand and stores
/// the result back in place. Equivalent to: r/m = 0 - r/m.
/// Sets CF if the operand was non-zero, clears CF otherwise.
/// Updates flags: OF, SF, ZF, AF, PF, CF.
///
/// Encoding reference:
///   NEG r/m8  → 0xF6  /3
///   NEG r/m16 → 0xF7  /3  (with 0x66 prefix)
///   NEG r/m32 → 0xF7  /3
///   NEG r/m64 → REX.W 0xF7  /3
struct NegPatterns {
  /// @brief NEG r/m — two's complement negate register or memory
  static OpcodePattern rm(OperandSize sz, uint8_t base);
};

// =============================================================================
// Logic / bitwise
// =============================================================================

/// @brief Factory for AND instruction patterns
///
/// @details Performs a bitwise AND of source and destination, stores the
/// result in destination
/// Clears OF and CF. Updates SF, ZF, PF. AF is undefined
///
/// Encoding reference:
///   AND r/m8,  r8    → 0x20  /r
///   AND r/m16, r16   → 0x21  /r  (with 0x66 prefix)
///   AND r/m32, r32   → 0x21  /r
///   AND r/m64, r64   → REX.W 0x21  /r
///   AND r8,    r/m8  → 0x22  /r
///   AND r16,   r/m16 → 0x23  /r  (with 0x66 prefix)
///   AND r32,   r/m32 → 0x23  /r
///   AND r64,   r/m64 → REX.W 0x23  /r
///   AND AL,    imm8  → 0x24  ib   (shortform, no ModRM)
///   AND r/m8,  imm8  → 0x80  /4  ib
///   AND r/m16, imm16 → 0x81  /4  iw
///   AND r/m32, imm32 → 0x81  /4  id
///   AND r/m64, imm32 → REX.W 0x81  /4  id  (sign-extended)
struct AndPatterns {
  /// @brief AND r/m, r — bitwise AND register into register or memory
  static OpcodePattern rm_r(OperandSize sz, uint8_t base);

  /// @brief AND r, r/m — bitwise AND register or memory into register
  static OpcodePattern r_rm(OperandSize sz, uint8_t base);

  /// @brief AND AL, imm8 — accumulator shortform, no ModRM (0x24)
  static OpcodePattern al_imm8(uint8_t base);

  /// @brief AND r/m, imm — bitwise AND immediate into register or memory
  /// @param modrm  Fixed REG field (/4 for AND)
  static OpcodePattern rm_imm(OperandSize sz, uint8_t base, uint8_t modrm);
};

/// @brief Factory for OR instruction patterns
///
/// @details Performs a bitwise OR of source and destination, stores the
/// result in destination
/// Clears OF and CF. Updates SF, ZF, PF. AF is undefined
///
/// Encoding reference:
///   OR r/m8,  r8    → 0x08  /r
///   OR r/m16, r16   → 0x09  /r  (with 0x66 prefix)
///   OR r/m32, r32   → 0x09  /r
///   OR r/m64, r64   → REX.W 0x09  /r
///   OR r8,    r/m8  → 0x0A  /r
///   OR r16,   r/m16 → 0x0B  /r  (with 0x66 prefix)
///   OR r32,   r/m32 → 0x0B  /r
///   OR r64,   r/m64 → REX.W 0x0B  /r
///   OR AL,    imm8  → 0x0C  ib   (shortform, no ModRM)
///   OR r/m8,  imm8  → 0x80  /1  ib
///   OR r/m16, imm16 → 0x81  /1  iw
///   OR r/m32, imm32 → 0x81  /1  id
///   OR r/m64, imm32 → REX.W 0x81  /1  id  (sign-extended)
struct OrPatterns {
  /// @brief OR r/m, r — bitwise OR register into register or memory
  static OpcodePattern rm_r(OperandSize sz, uint8_t base);

  /// @brief OR r, r/m — bitwise OR register or memory into register
  static OpcodePattern r_rm(OperandSize sz, uint8_t base);
  /// @brief OR AL, imm8 — accumulator shortform, no ModRM (0x0C)
  static OpcodePattern al_imm8(uint8_t base);

  /// @brief OR r/m, imm — bitwise OR immediate into register or memory
  /// @param modrm  Fixed REG field (/1 for OR)
  static OpcodePattern rm_imm(OperandSize sz, uint8_t base, uint8_t modrm);
};

/// @brief Factory for XOR instruction patterns
///
/// @details Performs a bitwise XOR of source and destination, stores the
/// result in destination.
/// Clears OF and CF. Updates SF, ZF, PF. AF is undefined.
///
/// Encoding reference:
///   XOR r/m8,  r8    → 0x30  /r
///   XOR r/m16, r16   → 0x31  /r  (with 0x66 prefix)
///   XOR r/m32, r32   → 0x31  /r
///   XOR r/m64, r64   → REX.W 0x31  /r
///   XOR r8,    r/m8  → 0x32  /r
///   XOR r16,   r/m16 → 0x33  /r  (with 0x66 prefix)
///   XOR r32,   r/m32 → 0x33  /r
///   XOR r64,   r/m64 → REX.W 0x33  /r
///   XOR AL,    imm8  → 0x34  ib   (shortform, no ModRM)
///   XOR r/m8,  imm8  → 0x80  /6  ib
///   XOR r/m16, imm16 → 0x81  /6  iw
///   XOR r/m32, imm32 → 0x81  /6  id
///   XOR r/m64, imm32 → REX.W 0x81  /6  id  (sign-extended)
struct XorPatterns {
  /// @brief XOR r/m, r — bitwise XOR register into register or memory
  static OpcodePattern rm_r(OperandSize sz, uint8_t base);

  /// @brief XOR r, r/m — bitwise XOR register or memory into register
  static OpcodePattern r_rm(OperandSize sz, uint8_t base);

  /// @brief XOR AL, imm8 — accumulator shortform, no ModRM (0x34)
  static OpcodePattern al_imm8(uint8_t base);

  /// @brief XOR r/m, imm — bitwise XOR immediate into register or memory
  /// @param modrm  Fixed REG field (/6 for XOR)
  static OpcodePattern rm_imm(OperandSize sz, uint8_t base, uint8_t modrm);
};

/// @brief Factory for NOT instruction patterns
///
/// @details Performs a bitwise one's complement of the operand and stores
/// the result back in place. Flips every bit: 0→1, 1→0.
/// Unlike NEG, NOT does not affect any flags at all.
///
/// Encoding reference:
///   NOT r/m8  → 0xF6  /2
///   NOT r/m16 → 0xF7  /2  (with 0x66 prefix)
///   NOT r/m32 → 0xF7  /2
///   NOT r/m64 → REX.W 0xF7  /2
struct NotPatterns {
  /// @brief NOT r/m — one's complement negate register or memory
  static OpcodePattern rm(OperandSize sz, uint8_t base);
};

/// @brief Factory for SHL instruction patterns
///
/// @details Shifts the destination left by the given count, filling the
/// vacated low bits with zeros. The last bit shifted out goes into CF.
/// SHL and SAL (Shift Arithmetic Left) are identical — same opcode, /4.
/// Updates CF, OF (only for count=1), SF, ZF, PF. AF is undefined.
///
/// Encoding reference:
///   SHL r/m8,  1    → 0xD0  /4
///   SHL r/m16, 1    → 0xD1  /4  (with 0x66 prefix)
///   SHL r/m32, 1    → 0xD1  /4
///   SHL r/m64, 1    → REX.W 0xD1  /4
///   SHL r/m8,  imm8 → 0xC0  /4  ib
///   SHL r/m16, imm8 → 0xC1  /4  ib  (with 0x66 prefix)
///   SHL r/m32, imm8 → 0xC1  /4  ib
///   SHL r/m64, imm8 → REX.W 0xC1  /4  ib
///   SHL r/m8,  CL   → 0xD2  /4
///   SHL r/m16, CL   → 0xD3  /4  (with 0x66 prefix)
///   SHL r/m32, CL   → 0xD3  /4
///   SHL r/m64, CL   → REX.W 0xD3  /4
struct ShlPatterns {
  /// @brief SHL r/m, 1 — shift left by implicit 1 (0xD0/D1 /4)
  static OpcodePattern rm_1(OperandSize sz, uint8_t base);

  /// @brief SHL r/m, imm8 — shift left by immediate count (0xC0/C1 /4)
  static OpcodePattern rm_imm8(OperandSize sz, uint8_t base);

  /// @brief SHL r/m, CL — shift left by count in CL (0xD2/D3 /4)
  static OpcodePattern rm_cl(OperandSize sz, uint8_t base);
};

/// @brief Factory for SHR instruction patterns
///
/// @details Logical shift right — shifts the destination right by the given
/// count, filling the vacated high bits with zeros regardless of the sign.
/// The last bit shifted out goes into CF.
/// Use SAR instead if sign-preservation is needed for signed values.
/// Updates CF, OF (only for count=1), SF, ZF, PF. AF is undefined.
///
/// Encoding reference:
///   SHR r/m8,  1    → 0xD0  /5
///   SHR r/m16, 1    → 0xD1  /5  (with 0x66 prefix)
///   SHR r/m32, 1    → 0xD1  /5
///   SHR r/m64, 1    → REX.W 0xD1  /5
///   SHR r/m8,  imm8 → 0xC0  /5  ib
///   SHR r/m16, imm8 → 0xC1  /5  ib  (with 0x66 prefix)
///   SHR r/m32, imm8 → 0xC1  /5  ib
///   SHR r/m64, imm8 → REX.W 0xC1  /5  ib
///   SHR r/m8,  CL   → 0xD2  /5
///   SHR r/m16, CL   → 0xD3  /5  (with 0x66 prefix)
///   SHR r/m32, CL   → 0xD3  /5
///   SHR r/m64, CL   → REX.W 0xD3  /5
struct ShrPatterns {
  /// @brief SHR r/m, 1 — logical shift right by implicit 1 (0xD0/D1 /5)
  static OpcodePattern rm_1(OperandSize sz, uint8_t base);

  /// @brief SHR r/m, imm8 — logical shift right by immediate count (0xC0/C1 /5)
  static OpcodePattern rm_imm8(OperandSize sz, uint8_t base);

  /// @brief SHR r/m, CL — logical shift right by count in CL (0xD2/D3 /5)
  static OpcodePattern rm_cl(OperandSize sz, uint8_t base);
};

/// @brief Factory for SAR instruction patterns
///
/// @details Arithmetic shift right — shifts the destination right by the
/// given count, filling the vacated high bits with copies of the sign bit.
/// This preserves the sign of a two's complement integer, making it
/// equivalent to signed division by a power of two (rounding toward -∞).
/// The last bit shifted out goes into CF.
/// Updates CF, OF (only for count=1), SF, ZF, PF. AF is undefined.
///
/// Encoding reference:
///   SAR r/m8,  1    → 0xD0  /7
///   SAR r/m16, 1    → 0xD1  /7  (with 0x66 prefix)
///   SAR r/m32, 1    → 0xD1  /7
///   SAR r/m64, 1    → REX.W 0xD1  /7
///   SAR r/m8,  imm8 → 0xC0  /7  ib
///   SAR r/m16, imm8 → 0xC1  /7  ib  (with 0x66 prefix)
///   SAR r/m32, imm8 → 0xC1  /7  ib
///   SAR r/m64, imm8 → REX.W 0xC1  /7  ib
///   SAR r/m8,  CL   → 0xD2  /7
///   SAR r/m16, CL   → 0xD3  /7  (with 0x66 prefix)
///   SAR r/m32, CL   → 0xD3  /7
///   SAR r/m64, CL   → REX.W 0xD3  /7
struct SarPatterns {
  /// @brief SAR r/m, 1 — arithmetic shift right by implicit 1 (0xD0/D1 /7)
  static OpcodePattern rm_1(OperandSize sz, uint8_t base);

  /// @brief SAR r/m, imm8 — arithmetic shift right by immediate count (0xC0/C1
  /// /7)
  static OpcodePattern rm_imm8(OperandSize sz, uint8_t base);

  /// @brief SAR r/m, CL — arithmetic shift right by count in CL (0xD2/D3 /7)
  static OpcodePattern rm_cl(OperandSize sz, uint8_t base);
};

// =============================================================================
// Compare / test
// =============================================================================

/// @brief Factory for CMP instruction patterns
///
/// @details Compares two operands by computing (destination - source) and
/// discarding the result. Only the flags are updated — neither operand
/// is modified. Structurally identical to SUB: same operand forms, same
/// ModRM layout, same immediate sizes — only the opcode bytes differ.
/// Updates flags: OF, SF, ZF, AF, CF, PF.
///
/// Encoding reference:
///   CMP r/m8,  r8    → 0x38  /r
///   CMP r/m16, r16   → 0x39  /r  (with 0x66 prefix)
///   CMP r/m32, r32   → 0x39  /r
///   CMP r/m64, r64   → REX.W 0x39  /r
///   CMP r8,    r/m8  → 0x3A  /r
///   CMP r16,   r/m16 → 0x3B  /r  (with 0x66 prefix)
///   CMP r32,   r/m32 → 0x3B  /r
///   CMP r64,   r/m64 → REX.W 0x3B  /r
///   CMP AL,    imm8  → 0x3C  ib   (shortform, no ModRM)
///   CMP r/m8,  imm8  → 0x80  /7  ib
///   CMP r/m16, imm16 → 0x81  /7  iw
///   CMP r/m32, imm32 → 0x81  /7  id
///   CMP r/m64, imm32 → REX.W 0x81  /7  id  (sign-extended)
struct CmpPatterns {
  /// @brief CMP r/m, r — compare register against register or memory
  static OpcodePattern rm_r(OperandSize sz, uint8_t base);

  /// @brief CMP r, r/m — compare register or memory against register
  static OpcodePattern r_rm(OperandSize sz, uint8_t base);

  /// @brief CMP AL, imm8 — accumulator shortform, no ModRM (0x3C)
  static OpcodePattern al_imm8(uint8_t base);

  /// @brief CMP r/m, imm — compare immediate against register or memory
  /// @param modrm  Fixed REG field (/7 for CMP)
  static OpcodePattern rm_imm(OperandSize sz, uint8_t base, uint8_t modrm);
};

/// @brief Factory for TEST instruction patterns
///
/// @details Performs a bitwise AND of the two operands and discards the
/// result. Only the flags are updated — neither operand is modified.
/// Structurally identical to AND: same operand forms, same ModRM layout —
/// only the opcode bytes and the absence of an r, r/m form differ.
/// Clears OF and CF. Updates SF, ZF, PF. AF is undefined.
///
/// Encoding reference:
///   TEST r/m8,  r8    → 0x84  /r
///   TEST r/m16, r16   → 0x85  /r  (with 0x66 prefix)
///   TEST r/m32, r32   → 0x85  /r
///   TEST r/m64, r64   → REX.W 0x85  /r
///   TEST AL,    imm8  → 0xA8  ib   (shortform, no ModRM)
///   TEST r/m8,  imm8  → 0xF6  /0  ib
///   TEST r/m16, imm16 → 0xF7  /0  iw
///   TEST r/m32, imm32 → 0xF7  /0  id
///   TEST r/m64, imm32 → REX.W 0xF7  /0  id  (sign-extended)
struct TestPatterns {
  /// @brief TEST r/m, r — bitwise AND to set flags, result discarded
  static OpcodePattern rm_r(OperandSize sz, uint8_t base);

  /// @brief TEST AL, imm8 — accumulator shortform, no ModRM (0xA8)
  static OpcodePattern al_imm8(uint8_t base);

  /// @brief TEST r/m, imm — bitwise AND immediate to set flags, result
  /// discarded
  /// @param modrm  Fixed REG field (/0 for TEST)
  static OpcodePattern rm_imm(OperandSize sz, uint8_t base, uint8_t modrm);
};

// =============================================================================
// Control flow
// =============================================================================

/// @brief Generic factory for conditional jump instructions (Jcc)
///
/// @details All conditional jumps share exactly the same structure —
/// one label/offset operand, no ModRM, only the opcode byte differs.
/// This single factory handles all of them: JE, JNE, JL, JLE, JG, JGE.
///
/// Encoding reference (short forms):
///   JE  / JZ  rel8  → 0x74  cb
///   JNE / JNZ rel8  → 0x75  cb
///   JL  / JNGE rel8 → 0x7C  cb
///   JLE / JNG  rel8 → 0x7E  cb
///   JG  / JNLE rel8 → 0x7F  cb
///   JGE / JNL  rel8 → 0x7D  cb
///
/// Encoding reference (near forms, 0x0F escape):
///   JE  / JZ  rel32 → 0x0F 0x84  cd
///   JNE / JNZ rel32 → 0x0F 0x85  cd
///   JL  / JNGE rel32→ 0x0F 0x8C  cd
///   JLE / JNG  rel32→ 0x0F 0x8E  cd
///   JG  / JNLE rel32→ 0x0F 0x8F  cd
///   JGE / JNL  rel32→ 0x0F 0x8D  cd
struct JccPatterns {
  /// @brief Jcc rel8 — short jump with 8-bit signed offset
  static OpcodePattern rel8(Opcode op, uint8_t base);

  /// @brief Jcc rel32 — near jump with 32-bit signed offset (0x0F escape)
  static OpcodePattern rel32(Opcode op, uint8_t base);
};

/// @brief Factory for JMP instruction patterns
///
/// @details Unconditional jump. Transfers control to the target address.
/// Unlike conditional jumps, JMP also supports indirect forms —
/// jumping to an address stored in a register or memory operand.
///
/// Encoding reference:
///   JMP rel8       → 0xEB  cb        short jump, 8-bit signed offset
///   JMP rel32      → 0xE9  cd        near jump, 32-bit signed offset
///   JMP r/m64      → 0xFF  /4        indirect jump via register or memory
struct JmpPatterns {
  /// @brief JMP rel8 — short unconditional jump (0xEB)
  static OpcodePattern rel8();

  /// @brief JMP rel32 — near unconditional jump (0xE9)
  static OpcodePattern rel32();

  /// @brief JMP r/m64 — indirect jump through register or memory (0xFF /4)
  static OpcodePattern rm64();
};

/// @brief Factory for CALL instruction patterns
///
/// @details Pushes the return address (next instruction) onto the stack,
/// then transfers control to the target. In 64-bit mode the return address
/// is always 64 bits.
///
/// Encoding reference:
///   CALL rel32  → 0xE8  cd     near call, 32-bit signed offset
///   CALL r/m64  → 0xFF  /2     indirect call via register or memory
struct CallPatterns {
  /// @brief CALL rel32 — near call with 32-bit signed offset (0xE8)
  static OpcodePattern rel32();

  /// @brief CALL r/m64 — indirect call through register or memory (0xFF /2)
  static OpcodePattern rm64();
};

/// @brief Factory for RET instruction patterns
///
/// @details Pops the return address from the stack and transfers control
/// to it. Optionally pops additional bytes from the stack after the return
/// address (used to clean up callee-cleaned calling conventions).
///
/// Encoding reference:
///   RET       → 0xC3        near return, no stack adjustment
///   RET imm16 → 0xC2  iw   near return, pops imm16 extra bytes from stack
struct RetPatterns {
  /// @brief RET — near return with no stack adjustment (0xC3)
  static OpcodePattern ret();

  /// @brief RET imm16 — near return, pop imm16 extra bytes after return address
  static OpcodePattern ret_imm16();
};

// =============================================================================
// Stack / frame
// =============================================================================
 
/// @brief Factory for ENTER instruction patterns
///
/// @details Creates a stack frame for a procedure. Equivalent to the
/// prologue sequence PUSH RBP / MOV RBP, RSP / SUB RSP, size, but
/// also handles nested lexical levels (nesting level > 0) by copying
/// frame pointers from outer frames — a feature used by Pascal-style
/// languages. In C/C++ the nesting level is always 0.
///
/// Encoding reference:
///   ENTER imm16, imm8 → 0xC8  iw  ib
struct EnterPatterns {
  /// @brief ENTER imm16, imm8 — allocate stack frame
  static OpcodePattern imm16_imm8();
};
 
/// @brief Factory for LEAVE instruction patterns
///
/// @details Collapses the current stack frame created by ENTER or a
/// manual prologue. Equivalent to: MOV RSP, RBP / POP RBP.
/// Takes no operands — always operates on RSP and RBP implicitly.
/// In 64-bit mode always uses 64-bit operand size.
///
/// Encoding reference:
///   LEAVE → 0xC9
struct LeavePatterns {
  /// @brief LEAVE — restore RSP and RBP from current stack frame
  static OpcodePattern leave();
};
 
// =============================================================================
// System / misc
// =============================================================================
 
/// @brief Factory for NOP instruction patterns
///
/// @details No operation — does nothing except consume one byte and
/// advance the instruction pointer. Commonly used for:
///   - Padding to align code to cache line boundaries
///   - Filling space left by deleted instructions (hot-patching)
///   - Inserting deliberate pipeline delays
///
/// Encoding reference:
///   NOP → 0x90
struct NopPatterns {
  /// @brief NOP — single-byte no operation
  static OpcodePattern nop();
};


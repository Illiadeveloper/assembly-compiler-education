/// @file OpcodeFactories.cpp
#include "opcodes/OpcodeFactories.h"

#include "common/OperandTypes.h"
#include "opcodes/OpcodePattern.h"

/// ============================================================================
/// MOV — Move
/// ============================================================================

OpcodePattern MovPatterns::rm_r(OperandSize sz, uint8_t base) {
  return OpcodePattern{
      Opcode::MOV,
      {{OK_REG | OK_MEM, sz, std::nullopt}, {OK_REG, sz, std::nullopt}},
      {base},
      true,
      std::nullopt,
      false,
      ExtraEncoding::NONE,
      {0, 0}};
}

OpcodePattern MovPatterns::r_rm(OperandSize sz, uint8_t base) {
  return OpcodePattern{
      Opcode::MOV,
      {{OK_REG, sz, std::nullopt}, {OK_REG | OK_MEM, sz, std::nullopt}},
      {base},
      true,
      std::nullopt,
      false,
      ExtraEncoding::NONE,
      {0, 0}};
}

OpcodePattern MovPatterns::r_imm(OperandSize sz, uint8_t base) {
  ExtraEncoding extra = (sz == OperandSize::B8)    ? ExtraEncoding::IMM8
                        : (sz == OperandSize::B16) ? ExtraEncoding::IMM16
                        : (sz == OperandSize::B32) ? ExtraEncoding::IMM32
                                                   : ExtraEncoding::IMM64;
  return OpcodePattern{
      Opcode::MOV,  {{OK_REG, sz, std::nullopt}, {OK_IMM, sz, std::nullopt}},
      {base},       false,
      std::nullopt,
      true,  // regInOpcode — B8+r
      extra,        {0, 1}};
}

OpcodePattern MovPatterns::rm_imm(OperandSize sz, uint8_t base) {
  ExtraEncoding extra = (sz == OperandSize::B8)    ? ExtraEncoding::IMM8
                        : (sz == OperandSize::B16) ? ExtraEncoding::IMM16
                                                   : ExtraEncoding::IMM32;
  return OpcodePattern{
      Opcode::MOV,
      {{OK_REG | OK_MEM, sz, std::nullopt}, {OK_IMM, sz, std::nullopt}},
      {base},
      true,
      uint8_t(0),  // /0
      false,
      extra,
      {0, 1}};
}

/// ============================================================================
/// LEA — Load Effective Address
/// ============================================================================
OpcodePattern LeaPatterns::r_m(OperandSize sz, uint8_t base) {
  return OpcodePattern{
      Opcode::LEA,
      {{OK_REG, sz, std::nullopt}, {OK_MEM, OperandSize::ANY, std::nullopt}},
      {base},
      true,
      std::nullopt,
      false,
      ExtraEncoding::NONE,
      {0, 1}};
}

/// ============================================================================
/// PUSH
/// ============================================================================
OpcodePattern PushPatterns::r(OperandSize sz, uint8_t base) {
  return OpcodePattern{Opcode::PUSH,
                       {{OK_REG, sz, std::nullopt}},
                       {base},
                       false,
                       std::nullopt,
                       true,  // 50+rd
                       ExtraEncoding::NONE,
                       {0}};
}

OpcodePattern PushPatterns::imm8(uint8_t base) {
  return OpcodePattern{Opcode::PUSH,
                       {{OK_IMM, OperandSize::B8, std::nullopt}},
                       {base},
                       false,
                       std::nullopt,
                       false,
                       ExtraEncoding::IMM8,
                       {0}};
}

// OpcodePattern PushPatterns::imm16(uint8_t base) {
//   return OpcodePattern{Opcode::PUSH,
//                        {{OK_IMM, OperandSize::B16, std::nullopt}},
//                        {base},
//                        false,
//                        std::nullopt,
//                        false,
//                        ExtraEncoding::IMM16,
//                        {0}};
// }

OpcodePattern PushPatterns::imm32(uint8_t base) {
  return OpcodePattern{Opcode::PUSH,
                       {{OK_IMM, OperandSize::B32, std::nullopt}},
                       {base},
                       false,
                       std::nullopt,
                       false,
                       ExtraEncoding::IMM32,
                       {0}};
}

OpcodePattern PushPatterns::rm(OperandSize sz, uint8_t base) {
  return OpcodePattern{Opcode::PUSH, {{OK_REG | OK_MEM, sz, std::nullopt}},
                       {base},       true,
                       uint8_t(6),  // /6
                       false,        ExtraEncoding::NONE,
                       {0}};
}

/// ============================================================================
/// POP
/// ============================================================================

OpcodePattern PopPatterns::r(OperandSize sz, uint8_t base) {
  return OpcodePattern{Opcode::POP,
                       {{OK_REG, sz, std::nullopt}},
                       {base},
                       false,
                       std::nullopt,
                       true,  // 58+rd
                       ExtraEncoding::NONE,
                       {0}};
}

OpcodePattern PopPatterns::rm(OperandSize sz, uint8_t base) {
  return OpcodePattern{Opcode::POP,  //
                       {{OK_REG | OK_MEM, sz, std::nullopt}},
                       {base},
                       true,
                       uint8_t(0),  // /0
                       false,
                       ExtraEncoding::NONE,
                       {0}};
}

/// ============================================================================
/// ADD — Integer Addition
/// ============================================================================

OpcodePattern AddPatterns::rm_r(OperandSize sz, uint8_t base) {
  return OpcodePattern{
      Opcode::ADD,
      {{OK_REG | OK_MEM, sz, std::nullopt}, {OK_REG, sz, std::nullopt}},
      {base},
      true,
      std::nullopt,
      false,
      ExtraEncoding::NONE,
      {0, 0}};
}

OpcodePattern AddPatterns::r_rm(OperandSize sz, uint8_t base) {
  return OpcodePattern{
      Opcode::ADD,
      {{OK_REG, sz, std::nullopt}, {OK_REG | OK_MEM, sz, std::nullopt}},
      {base},
      true,
      std::nullopt,
      false,
      ExtraEncoding::NONE,
      {0, 0}};
}

OpcodePattern AddPatterns::al_imm8(uint8_t base) {
  return OpcodePattern{Opcode::ADD,
                       {{OK_REG, OperandSize::B8, Register::AL},
                        {OK_IMM, OperandSize::B8, std::nullopt}},
                       {base},
                       false,
                       std::nullopt,
                       false,
                       ExtraEncoding::NONE,
                       {0, 0}};
}

OpcodePattern AddPatterns::rm_imm(OperandSize sz, uint8_t base, uint8_t modrm) {
  ExtraEncoding extra = (sz == OperandSize::B8)    ? ExtraEncoding::IMM8
                        : (sz == OperandSize::B16) ? ExtraEncoding::IMM16
                                                   : ExtraEncoding::IMM32;
  return OpcodePattern{
      Opcode::ADD,
      {{OK_REG | OK_MEM, sz, std::nullopt}, {OK_IMM, sz, std::nullopt}},
      {base},
      true,
      modrm,
      false,
      extra,
      {0, 1}};
}

/// ============================================================================
/// SUB — Integer Subtraction
/// ============================================================================

OpcodePattern SubPatterns::rm_r(OperandSize sz, uint8_t base) {
  return OpcodePattern{
      Opcode::SUB,
      {{OK_REG | OK_MEM, sz, std::nullopt}, {OK_REG, sz, std::nullopt}},
      {base},
      true,
      std::nullopt,
      false,
      ExtraEncoding::NONE,
      {0, 0}};
}

OpcodePattern SubPatterns::r_rm(OperandSize sz, uint8_t base) {
  return OpcodePattern{
      Opcode::SUB,
      {{OK_REG, sz, std::nullopt}, {OK_REG | OK_MEM, sz, std::nullopt}},
      {base},
      true,
      std::nullopt,
      false,
      ExtraEncoding::NONE,
      {0, 0}};
}

OpcodePattern SubPatterns::al_imm8(uint8_t base) {
  return OpcodePattern{Opcode::SUB,
                       {{OK_REG, OperandSize::B8, Register::AL},
                        {OK_IMM, OperandSize::B8, std::nullopt}},
                       {base},
                       false,
                       std::nullopt,
                       false,
                       ExtraEncoding::IMM8,
                       {0, 1}};
}

OpcodePattern SubPatterns::rm_imm(OperandSize sz, uint8_t base, uint8_t modrm) {
  ExtraEncoding extra = (sz == OperandSize::B8)    ? ExtraEncoding::IMM8
                        : (sz == OperandSize::B16) ? ExtraEncoding::IMM16
                                                   : ExtraEncoding::IMM32;
  return OpcodePattern{
      Opcode::SUB,
      {{OK_REG | OK_MEM, sz, std::nullopt}, {OK_IMM, sz, std::nullopt}},
      {base},
      true,
      modrm,
      false,
      extra,
      {0, 1}};
}

/// ============================================================================
/// IMUL — Signed Multiplication
/// ============================================================================

OpcodePattern ImulPatterns::rm(OperandSize sz, uint8_t base) {
  // One-operand form: IMUL r/m  (result in rDX:rAX)
  return OpcodePattern{Opcode::IMUL, {{OK_REG | OK_MEM, sz, std::nullopt}},
                       {base},       true,
                       uint8_t(5),  // /5
                       false,        ExtraEncoding::NONE,
                       {0}};
}

OpcodePattern ImulPatterns::r_rm(OperandSize sz) {
  // Two-operand form: IMUL r, r/m  (0F AF /r)
  return OpcodePattern{
      Opcode::IMUL,
      {{OK_REG, sz, std::nullopt}, {OK_REG | OK_MEM, sz, std::nullopt}},
      {0x0F, 0xAF},
      true,
      std::nullopt,
      false,
      ExtraEncoding::NONE,
      {0, 0}};
}

OpcodePattern ImulPatterns::r_rm_imm8(OperandSize sz) {
  // Three-operand form with sign-extended imm8: IMUL r, r/m, imm8  (6B /r ib)
  return OpcodePattern{Opcode::IMUL,
                       {{OK_REG, sz, std::nullopt},
                        {OK_REG | OK_MEM, sz, std::nullopt},
                        {OK_IMM, OperandSize::B8, std::nullopt}},
                       {0x6B},
                       true,
                       std::nullopt,
                       false,
                       ExtraEncoding::IMM8_SIGNED,
                       {0, 0, 2}};
}

OpcodePattern ImulPatterns::r_rm_imm(OperandSize sz) {
  // Three-operand form: IMUL r, r/m, imm16/imm32  (69 /r iw/id)
  ExtraEncoding extra =
      (sz == OperandSize::B16) ? ExtraEncoding::IMM16 : ExtraEncoding::IMM32;
  return OpcodePattern{Opcode::IMUL,
                       {{OK_REG, sz, std::nullopt},
                        {OK_REG | OK_MEM, sz, std::nullopt},
                        {OK_IMM, sz, std::nullopt}},
                       {0x69},
                       true,
                       std::nullopt,
                       false,
                       extra,
                       {0, 0, 2}};
}

/// ============================================================================
/// MUL — Unsigned Multiplication
/// ============================================================================

OpcodePattern MulPatterns::rm(OperandSize sz, uint8_t base) {
  return OpcodePattern{Opcode::MUL, {{OK_REG | OK_MEM, sz, std::nullopt}},
                       {base},      true,
                       uint8_t(4),  // /4
                       false,       ExtraEncoding::NONE,
                       {0}};
}

/// ============================================================================
/// IDIV — Signed Division
/// ============================================================================

OpcodePattern IdivPatterns::rm(OperandSize sz, uint8_t base) {
  return OpcodePattern{Opcode::IDIV, {{OK_REG | OK_MEM, sz, std::nullopt}},
                       {base},       true,
                       uint8_t(7),  // /7
                       false,        ExtraEncoding::NONE,
                       {0}};
}

/// ============================================================================
/// DIV — Unsigned Division
/// ============================================================================

OpcodePattern DivPatterns::rm(OperandSize sz, uint8_t base) {
  return OpcodePattern{Opcode::DIV, {{OK_REG | OK_MEM, sz, std::nullopt}},
                       {base},      true,
                       uint8_t(6),  // /6
                       false,       ExtraEncoding::NONE,
                       {0}};
}

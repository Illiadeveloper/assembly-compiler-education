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

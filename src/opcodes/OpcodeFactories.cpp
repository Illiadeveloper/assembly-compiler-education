#include "opcodes/OpcodeFactories.h"

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
  return OpcodePattern{
      Opcode::ADD,
      {{OK_REG | OK_MEM, sz, std::nullopt}, {OK_IMM, sz, std::nullopt}},
      {base},
      true,
      modrm,
      false,
      ExtraEncoding::NONE,
      {0, 0}};
}

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

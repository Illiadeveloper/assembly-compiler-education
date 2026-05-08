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

/// @brief Factory for ADD instruction patterns
struct AddPatterns {
  /// @brief ADD r/m, r — register/memory to register variant
  static OpcodePattern rm_r(OperandSize sz, uint8_t base);

  /// @brief ADD r, r/m — register to register/memory variant
  static OpcodePattern r_rm(OperandSize sz, uint8_t base);

  /// @brief ADD AL, imm8 — accumulator with immediate
  static OpcodePattern al_imm8(uint8_t base);

  /// @brief ADD r/m, imm — register/memory with immediate
  static OpcodePattern rm_imm(OperandSize sz, uint8_t base, uint8_t modrm);
};

/// @brief Factory for MOV instruction patterns
struct MovPatterns {
  /// @brief MOV r/m, r — memory/register from register
  static OpcodePattern rm_r(OperandSize sz, uint8_t base);

  /// @brief MOV r, r/m — register from memory/register  
  static OpcodePattern r_rm(OperandSize sz, uint8_t base);

  /// @brief MOV r, imm — register from immediate (B8+r form)
  static OpcodePattern r_imm(OperandSize sz, uint8_t base);

  /// @brief MOV r/m, imm — memory/register from immediate
  static OpcodePattern rm_imm(OperandSize sz, uint8_t base);
};

///
/// @file OpcodeTable.h
/// @brief Encoding table of all opcodes
#pragma once
#include <array>
#include <vector>

#include "Opcode.h"
#include "OpcodeFactories.h"
#include "OpcodePattern.h"

/// @brief Global opcode encoding table
///
/// @details
/// This table maps each Opcode to a list of possible encoding patterns.
/// Each pattern describes a specific instruction form (operand types,
/// sizes, encoding bytes, ModRM usage, etc.).
///
/// Design notes:
/// - Multiple patterns per opcode allow handling instruction variants
///   (e.g., ADD r/m8, r8 vs ADD r8, r/m8).
/// - Operand matching is performed at runtime by checking constraints.
///
/// Encoding conventions:
/// - baseBytes: opcode bytes without ModRM/immediates
/// - hasModRM: whether ModRM byte must be emitted
/// - modrm_reg: fixed REG field (/0, /1, etc.)
/// - regInOpcode: register encoded in opcode (e.g., B8 + r)
/// - extra: additional encoding (immediate, relative offsets)
///
/// Example:
///   ADD r/m8, r8  → opcode 0x00, ModRM required
///   ADD AL, imm8  → opcode 0x04, no ModRM
static const std::array<std::vector<OpcodePattern>, opcode_count> OpcodeTable =
    [] {
      std::array<std::vector<OpcodePattern>, opcode_count> table{};
      /// ============================================================================
      /// MOV — Move
      /// ============================================================================
      table[static_cast<size_t>(Opcode::MOV)] = {
          // MOV r/m, r
          MovPatterns::rm_r(OperandSize::B8, 0x88),
          MovPatterns::rm_r(OperandSize::B16, 0x89),
          MovPatterns::rm_r(OperandSize::B32, 0x89),
          MovPatterns::rm_r(OperandSize::B64, 0x89),

          // MOV r, r/m
          MovPatterns::r_rm(OperandSize::B8, 0x8A),
          MovPatterns::r_rm(OperandSize::B16, 0x8B),
          MovPatterns::r_rm(OperandSize::B32, 0x8B),
          MovPatterns::r_rm(OperandSize::B64, 0x8B),

          // MOV r, imm  (B8+r form)
          MovPatterns::r_imm(OperandSize::B8, 0xB0),
          MovPatterns::r_imm(OperandSize::B16, 0xB8),
          MovPatterns::r_imm(OperandSize::B32, 0xB8),
          MovPatterns::r_imm(OperandSize::B64, 0xB8),

          // MOV r/m, imm  (C6/C7 form)
          MovPatterns::rm_imm(OperandSize::B8, 0xC6),
          MovPatterns::rm_imm(OperandSize::B16, 0xC7),
          MovPatterns::rm_imm(OperandSize::B32, 0xC7),
          MovPatterns::rm_imm(OperandSize::B64, 0xC7),
      };

      /// ============================================================================
      /// LEA — Load Effective Address
      /// ============================================================================
      table[static_cast<size_t>(Opcode::LEA)] = {
          LeaPatterns::r_m(OperandSize::B16, 0x8D),
          LeaPatterns::r_m(OperandSize::B32, 0x8D),
          LeaPatterns::r_m(OperandSize::B64, 0x8D),
      };

      /// ============================================================================
      /// PUSH
      /// ============================================================================
      table[static_cast<size_t>(Opcode::PUSH)] = {
          // 50+rd
          PushPatterns::r(OperandSize::B16, 0x50),  // push r16
          PushPatterns::r(OperandSize::B64, 0x50),  // push r64

          // immediate
          PushPatterns::imm8(0x6A),  // push imm8  (sign-extended)
          // PushPatterns::imm16(0x68),   // push imm16  (sign-extended)
          PushPatterns::imm32(0x68),  // push imm32 (sign-extended)

          // FF /6
          PushPatterns::rm(OperandSize::B16, 0xFF),  // push r/m16
          PushPatterns::rm(OperandSize::B64, 0xFF),  // push r/m64
      };

      /// ============================================================================
      /// POP
      /// ============================================================================
      table[static_cast<size_t>(Opcode::POP)] = {
          PopPatterns::r(OperandSize::B64, 0x58),   // pop r64 → 58+rd
          PopPatterns::r(OperandSize::B16, 0x58),   // pop r16 → 58+rd
          PopPatterns::rm(OperandSize::B64, 0x8F),  // 8F /0
          PopPatterns::rm(OperandSize::B16, 0x8F),  // 8F /0
      };

      /// ============================================================================
      /// ADD — Integer Addition
      /// ============================================================================
      table[static_cast<size_t>(Opcode::ADD)] = {
          AddPatterns::rm_r(OperandSize::B8, 0x00),
          AddPatterns::rm_r(OperandSize::B16, 0x01),
          AddPatterns::rm_r(OperandSize::B32, 0x01),
          AddPatterns::rm_r(OperandSize::B64, 0x01),

          AddPatterns::r_rm(OperandSize::B8, 0x02),
          AddPatterns::r_rm(OperandSize::B16, 0x03),
          AddPatterns::r_rm(OperandSize::B32, 0x03),
          AddPatterns::r_rm(OperandSize::B64, 0x03),

          AddPatterns::al_imm8(0x04),

          AddPatterns::rm_imm(OperandSize::B8, 0x80, 0),

          AddPatterns::rm_imm(OperandSize::B16, 0x81,
                              0),  // 81 /0 — r/m16, imm16
          AddPatterns::rm_imm(OperandSize::B32, 0x81,
                              0),  // 81 /0 — r/m32, imm32
          AddPatterns::rm_imm(OperandSize::B64, 0x81,
                              0),  // 81 /0 — r/m64, imm32 (sign-extended)
      };

      /// ============================================================================
      /// SUB — Integer Subtraction
      /// ============================================================================
      table[static_cast<size_t>(Opcode::SUB)] = {
          // SUB r/m, r
          SubPatterns::rm_r(OperandSize::B8, 0x28),
          SubPatterns::rm_r(OperandSize::B16, 0x29),
          SubPatterns::rm_r(OperandSize::B32, 0x29),
          SubPatterns::rm_r(OperandSize::B64, 0x29),

          // SUB r, r/m
          SubPatterns::r_rm(OperandSize::B8, 0x2A),
          SubPatterns::r_rm(OperandSize::B16, 0x2B),
          SubPatterns::r_rm(OperandSize::B32, 0x2B),
          SubPatterns::r_rm(OperandSize::B64, 0x2B),

          // SUB AL, imm8  (shortform, no ModRM)
          SubPatterns::al_imm8(0x2C),

          // SUB r/m, imm  (80/81 /5)
          SubPatterns::rm_imm(OperandSize::B8, 0x80, 5),
          SubPatterns::rm_imm(OperandSize::B16, 0x81, 5),
          SubPatterns::rm_imm(OperandSize::B32, 0x81, 5),
          SubPatterns::rm_imm(OperandSize::B64, 0x81, 5),
      };

      /// ============================================================================
      /// IMUL — Signed Multiplication
      /// ============================================================================
      table[static_cast<size_t>(Opcode::IMUL)] = {
          // One-operand: IMUL r/m  →  F6 /5 / F7 /5
          ImulPatterns::rm(OperandSize::B8, 0xF6),
          ImulPatterns::rm(OperandSize::B16, 0xF7),
          ImulPatterns::rm(OperandSize::B32, 0xF7),
          ImulPatterns::rm(OperandSize::B64, 0xF7),

          // Two-operand: IMUL r, r/m  →  0F AF /r
          ImulPatterns::r_rm(OperandSize::B16),
          ImulPatterns::r_rm(OperandSize::B32),
          ImulPatterns::r_rm(OperandSize::B64),

          // Three-operand (imm8 sign-extended): IMUL r, r/m, imm8  →  6B /r ib
          ImulPatterns::r_rm_imm8(OperandSize::B16),
          ImulPatterns::r_rm_imm8(OperandSize::B32),
          ImulPatterns::r_rm_imm8(OperandSize::B64),

          // Three-operand (imm16/imm32): IMUL r, r/m, imm  →  69 /r iw/id
          ImulPatterns::r_rm_imm(OperandSize::B16),
          ImulPatterns::r_rm_imm(OperandSize::B32),
          ImulPatterns::r_rm_imm(OperandSize::B64),
      };

      /// ============================================================================
      /// MUL — Unsigned Multiplication
      /// ============================================================================
      table[static_cast<size_t>(Opcode::MUL)] = {
          // MUL r/m  →  F6 /4 / F7 /4
          MulPatterns::rm(OperandSize::B8, 0xF6),
          MulPatterns::rm(OperandSize::B16, 0xF7),
          MulPatterns::rm(OperandSize::B32, 0xF7),
          MulPatterns::rm(OperandSize::B64, 0xF7),
      };

      /// ============================================================================
      /// IDIV — Signed Division
      /// ============================================================================
      table[static_cast<size_t>(Opcode::IDIV)] = {
          // IDIV r/m  →  F6 /7 / F7 /7
          IdivPatterns::rm(OperandSize::B8, 0xF6),
          IdivPatterns::rm(OperandSize::B16, 0xF7),
          IdivPatterns::rm(OperandSize::B32, 0xF7),
          IdivPatterns::rm(OperandSize::B64, 0xF7),
      };

      /// ============================================================================
      /// DIV — Unsigned Division
      /// ============================================================================
      table[static_cast<size_t>(Opcode::DIV)] = {
          // DIV r/m  →  F6 /6 / F7 /6
          DivPatterns::rm(OperandSize::B8, 0xF6),
          DivPatterns::rm(OperandSize::B16, 0xF7),
          DivPatterns::rm(OperandSize::B32, 0xF7),
          DivPatterns::rm(OperandSize::B64, 0xF7),
      };

      /// ============================================================================
      /// SYSCALL
      /// ============================================================================
      table[static_cast<size_t>(Opcode::SYSCALL)] = {
          OpcodePattern{Opcode::SYSCALL,
                        {},  // doens't have operands
                        {0x0F, 0x05},
                        false,
                        std::nullopt,
                        false,
                        ExtraEncoding::NONE,
                        {}}};

      return table;
    }();

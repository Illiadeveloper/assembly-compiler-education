/// @file OpcodeTable.h
/// @brief Encoding table of all opcodes
#pragma once
#include "Opcode.h"
#include "OpcodeFactories.h"
#include "OpcodePattern.h"
#include "common/Registers.h"
#include <array>
#include <vector>

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
      /// ADD — Integer Addition
      /// ============================================================================
      table[static_cast<size_t>(Opcode::ADD)] = {
          AddPatterns::rm_r(OperandSize::B8, 0x00),
          AddPatterns::rm_r(OperandSize::B16, 0x01),
          AddPatterns::rm_r(OperandSize::B32, 0x01),
          AddPatterns::r_rm(OperandSize::B8, 0x02),
          AddPatterns::al_imm8(0x04),
          AddPatterns::rm_imm(OperandSize::B8, 0x80, 0)};

      // MOV (late)
      // table[static_cast<size_t>(Opcode::MOV)] = { ... };

      return table;
    }();

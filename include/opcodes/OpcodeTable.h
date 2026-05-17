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

          // LEA r, label — RIP-relative address of label
          LeaPatterns::r_label(OperandSize::B32),
          LeaPatterns::r_label(OperandSize::B64),
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
      /// INC — Increment by 1
      /// ============================================================================
      table[static_cast<size_t>(Opcode::INC)] = {
          // INC r/m  →  FE /0 / FF /0
          IncPatterns::rm(OperandSize::B8, 0xFE),
          IncPatterns::rm(OperandSize::B16, 0xFF),
          IncPatterns::rm(OperandSize::B32, 0xFF),
          IncPatterns::rm(OperandSize::B64, 0xFF),
      };

      /// ============================================================================
      /// DEC — Decrement by 1
      /// ============================================================================
      table[static_cast<size_t>(Opcode::DEC)] = {
          // DEC r/m  →  FE /1 / FF /1
          DecPatterns::rm(OperandSize::B8, 0xFE),
          DecPatterns::rm(OperandSize::B16, 0xFF),
          DecPatterns::rm(OperandSize::B32, 0xFF),
          DecPatterns::rm(OperandSize::B64, 0xFF),
      };

      /// ============================================================================
      /// NEG — Two's Complement Negation
      /// ============================================================================
      table[static_cast<size_t>(Opcode::NEG)] = {
          // NEG r/m  →  F6 /3 / F7 /3
          NegPatterns::rm(OperandSize::B8, 0xF6),
          NegPatterns::rm(OperandSize::B16, 0xF7),
          NegPatterns::rm(OperandSize::B32, 0xF7),
          NegPatterns::rm(OperandSize::B64, 0xF7),
      };

      /// ============================================================================
      /// AND — Bitwise AND
      /// ============================================================================
      table[static_cast<size_t>(Opcode::AND)] = {
          // AND r/m, r
          AndPatterns::rm_r(OperandSize::B8, 0x20),
          AndPatterns::rm_r(OperandSize::B16, 0x21),
          AndPatterns::rm_r(OperandSize::B32, 0x21),
          AndPatterns::rm_r(OperandSize::B64, 0x21),

          // AND r, r/m
          AndPatterns::r_rm(OperandSize::B8, 0x22),
          AndPatterns::r_rm(OperandSize::B16, 0x23),
          AndPatterns::r_rm(OperandSize::B32, 0x23),
          AndPatterns::r_rm(OperandSize::B64, 0x23),

          // AND AL, imm8  (shortform 0x24)
          AndPatterns::al_imm8(0x24),

          // AND r/m, imm  (0x80/0x81 /4)
          AndPatterns::rm_imm(OperandSize::B8, 0x80, 4),
          AndPatterns::rm_imm(OperandSize::B16, 0x81, 4),
          AndPatterns::rm_imm(OperandSize::B32, 0x81, 4),
          AndPatterns::rm_imm(OperandSize::B64, 0x81, 4),
      };

      /// ============================================================================
      /// OR — Bitwise OR
      /// ============================================================================
      table[static_cast<size_t>(Opcode::OR)] = {
          // OR r/m, r
          OrPatterns::rm_r(OperandSize::B8, 0x08),
          OrPatterns::rm_r(OperandSize::B16, 0x09),
          OrPatterns::rm_r(OperandSize::B32, 0x09),
          OrPatterns::rm_r(OperandSize::B64, 0x09),

          // OR r, r/m
          OrPatterns::r_rm(OperandSize::B8, 0x0A),
          OrPatterns::r_rm(OperandSize::B16, 0x0B),
          OrPatterns::r_rm(OperandSize::B32, 0x0B),
          OrPatterns::r_rm(OperandSize::B64, 0x0B),

          // OR AL, imm8  (shortform 0x0C)
          OrPatterns::al_imm8(0x0C),

          // OR r/m, imm  (0x80/0x81 /1)
          OrPatterns::rm_imm(OperandSize::B8, 0x80, 1),
          OrPatterns::rm_imm(OperandSize::B16, 0x81, 1),
          OrPatterns::rm_imm(OperandSize::B32, 0x81, 1),
          OrPatterns::rm_imm(OperandSize::B64, 0x81, 1),
      };

      /// ============================================================================
      /// XOR — Bitwise XOR
      /// ============================================================================
      table[static_cast<size_t>(Opcode::XOR)] = {
          // XOR r/m, r
          XorPatterns::rm_r(OperandSize::B8, 0x30),
          XorPatterns::rm_r(OperandSize::B16, 0x31),
          XorPatterns::rm_r(OperandSize::B32, 0x31),
          XorPatterns::rm_r(OperandSize::B64, 0x31),

          // XOR r, r/m
          XorPatterns::r_rm(OperandSize::B8, 0x32),
          XorPatterns::r_rm(OperandSize::B16, 0x33),
          XorPatterns::r_rm(OperandSize::B32, 0x33),
          XorPatterns::r_rm(OperandSize::B64, 0x33),

          // XOR AL, imm8  (shortform 0x34)
          XorPatterns::al_imm8(0x34),

          // XOR r/m, imm  (0x80/0x81 /6)
          XorPatterns::rm_imm(OperandSize::B8, 0x80, 6),
          XorPatterns::rm_imm(OperandSize::B16, 0x81, 6),
          XorPatterns::rm_imm(OperandSize::B32, 0x81, 6),
          XorPatterns::rm_imm(OperandSize::B64, 0x81, 6),
      };

      /// ============================================================================
      /// NOT — Bitwise One's Complement
      /// ============================================================================
      table[static_cast<size_t>(Opcode::NOT)] = {
          // NOT r/m  →  F6 /2 / F7 /2
          NotPatterns::rm(OperandSize::B8, 0xF6),
          NotPatterns::rm(OperandSize::B16, 0xF7),
          NotPatterns::rm(OperandSize::B32, 0xF7),
          NotPatterns::rm(OperandSize::B64, 0xF7),
      };

      /// ============================================================================
      /// SHL — Shift Left
      /// ============================================================================
      table[static_cast<size_t>(Opcode::SHL)] = {
          // SHL r/m, 1  →  D0 /4 / D1 /4
          ShlPatterns::rm_1(OperandSize::B8, 0xD0),
          ShlPatterns::rm_1(OperandSize::B16, 0xD1),
          ShlPatterns::rm_1(OperandSize::B32, 0xD1),
          ShlPatterns::rm_1(OperandSize::B64, 0xD1),

          // SHL r/m, imm8  →  C0 /4 / C1 /4
          ShlPatterns::rm_imm8(OperandSize::B8, 0xC0),
          ShlPatterns::rm_imm8(OperandSize::B16, 0xC1),
          ShlPatterns::rm_imm8(OperandSize::B32, 0xC1),
          ShlPatterns::rm_imm8(OperandSize::B64, 0xC1),

          // SHL r/m, CL  →  D2 /4 / D3 /4
          ShlPatterns::rm_cl(OperandSize::B8, 0xD2),
          ShlPatterns::rm_cl(OperandSize::B16, 0xD3),
          ShlPatterns::rm_cl(OperandSize::B32, 0xD3),
          ShlPatterns::rm_cl(OperandSize::B64, 0xD3),
      };

      /// ============================================================================
      /// SHR — Logical Shift Right
      /// ============================================================================
      table[static_cast<size_t>(Opcode::SHR)] = {
          // SHR r/m, 1  →  D0 /5 / D1 /5
          ShrPatterns::rm_1(OperandSize::B8, 0xD0),
          ShrPatterns::rm_1(OperandSize::B16, 0xD1),
          ShrPatterns::rm_1(OperandSize::B32, 0xD1),
          ShrPatterns::rm_1(OperandSize::B64, 0xD1),

          // SHR r/m, imm8  →  C0 /5 / C1 /5
          ShrPatterns::rm_imm8(OperandSize::B8, 0xC0),
          ShrPatterns::rm_imm8(OperandSize::B16, 0xC1),
          ShrPatterns::rm_imm8(OperandSize::B32, 0xC1),
          ShrPatterns::rm_imm8(OperandSize::B64, 0xC1),

          // SHR r/m, CL  →  D2 /5 / D3 /5
          ShrPatterns::rm_cl(OperandSize::B8, 0xD2),
          ShrPatterns::rm_cl(OperandSize::B16, 0xD3),
          ShrPatterns::rm_cl(OperandSize::B32, 0xD3),
          ShrPatterns::rm_cl(OperandSize::B64, 0xD3),
      };

      /// ============================================================================
      /// SAR — Arithmetic Shift Right
      /// ============================================================================
      table[static_cast<size_t>(Opcode::SAR)] = {
          // SAR r/m, 1  →  D0 /7 / D1 /7
          SarPatterns::rm_1(OperandSize::B8, 0xD0),
          SarPatterns::rm_1(OperandSize::B16, 0xD1),
          SarPatterns::rm_1(OperandSize::B32, 0xD1),
          SarPatterns::rm_1(OperandSize::B64, 0xD1),

          // SAR r/m, imm8  →  C0 /7 / C1 /7
          SarPatterns::rm_imm8(OperandSize::B8, 0xC0),
          SarPatterns::rm_imm8(OperandSize::B16, 0xC1),
          SarPatterns::rm_imm8(OperandSize::B32, 0xC1),
          SarPatterns::rm_imm8(OperandSize::B64, 0xC1),

          // SAR r/m, CL  →  D2 /7 / D3 /7
          SarPatterns::rm_cl(OperandSize::B8, 0xD2),
          SarPatterns::rm_cl(OperandSize::B16, 0xD3),
          SarPatterns::rm_cl(OperandSize::B32, 0xD3),
          SarPatterns::rm_cl(OperandSize::B64, 0xD3),
      };

      /// ============================================================================
      /// CMP — Compare
      /// ============================================================================
      table[static_cast<size_t>(Opcode::CMP)] = {
          // CMP r/m, r
          CmpPatterns::rm_r(OperandSize::B8, 0x38),
          CmpPatterns::rm_r(OperandSize::B16, 0x39),
          CmpPatterns::rm_r(OperandSize::B32, 0x39),
          CmpPatterns::rm_r(OperandSize::B64, 0x39),

          // CMP r, r/m
          CmpPatterns::r_rm(OperandSize::B8, 0x3A),
          CmpPatterns::r_rm(OperandSize::B16, 0x3B),
          CmpPatterns::r_rm(OperandSize::B32, 0x3B),
          CmpPatterns::r_rm(OperandSize::B64, 0x3B),

          // CMP AL, imm8  (shortform 0x3C)
          CmpPatterns::al_imm8(0x3C),

          // CMP r/m, imm  (0x80/0x81 /7)
          CmpPatterns::rm_imm(OperandSize::B8, 0x80, 7),
          CmpPatterns::rm_imm(OperandSize::B16, 0x81, 7),
          CmpPatterns::rm_imm(OperandSize::B32, 0x81, 7),
          CmpPatterns::rm_imm(OperandSize::B64, 0x81, 7),
      };

      /// ============================================================================
      /// TEST — Bitwise AND without storing result
      /// ============================================================================
      table[static_cast<size_t>(Opcode::TEST)] = {
          // TEST r/m, r
          TestPatterns::rm_r(OperandSize::B8, 0x84),
          TestPatterns::rm_r(OperandSize::B16, 0x85),
          TestPatterns::rm_r(OperandSize::B32, 0x85),
          TestPatterns::rm_r(OperandSize::B64, 0x85),

          // TEST AL, imm8  (shortform 0xA8)
          TestPatterns::al_imm8(0xA8),

          // TEST r/m, imm  (0xF6/0xF7 /0)
          TestPatterns::rm_imm(OperandSize::B8, 0xF6, 0),
          TestPatterns::rm_imm(OperandSize::B16, 0xF7, 0),
          TestPatterns::rm_imm(OperandSize::B32, 0xF7, 0),
          TestPatterns::rm_imm(OperandSize::B64, 0xF7, 0),
      };

      /// ============================================================================
      /// JMP — Unconditional Jump
      /// ============================================================================
      table[static_cast<size_t>(Opcode::JMP)] = {
          JmpPatterns::rel8(),   // 0xEB  cb
          JmpPatterns::rel32(),  // 0xE9  cd
          JmpPatterns::rm64(),   // 0xFF /4
      };

      /// ============================================================================
      /// JE — Jump if Equal (ZF=1)
      /// ============================================================================
      table[static_cast<size_t>(Opcode::JE)] = {
          JccPatterns::rel8(Opcode::JE, 0x74),   // 0x74      cb
          JccPatterns::rel32(Opcode::JE, 0x84),  // 0x0F 0x84 cd
      };

      /// ============================================================================
      /// JNE — Jump if Not Equal (ZF=0)
      /// ============================================================================
      table[static_cast<size_t>(Opcode::JNE)] = {
          JccPatterns::rel8(Opcode::JNE, 0x75),   // 0x75      cb
          JccPatterns::rel32(Opcode::JNE, 0x85),  // 0x0F 0x85 cd
      };

      /// ============================================================================
      /// JL — Jump if Less (SF≠OF)
      /// ============================================================================
      table[static_cast<size_t>(Opcode::JL)] = {
          JccPatterns::rel8(Opcode::JL, 0x7C),   // 0x7C      cb
          JccPatterns::rel32(Opcode::JL, 0x8C),  // 0x0F 0x8C cd
      };

      /// ============================================================================
      /// JLE — Jump if Less or Equal (ZF=1 or SF≠OF)
      /// ============================================================================
      table[static_cast<size_t>(Opcode::JLE)] = {
          JccPatterns::rel8(Opcode::JLE, 0x7E),   // 0x7E      cb
          JccPatterns::rel32(Opcode::JLE, 0x8E),  // 0x0F 0x8E cd
      };

      /// ============================================================================
      /// JG — Jump if Greater (ZF=0 and SF=OF)
      /// ============================================================================
      table[static_cast<size_t>(Opcode::JG)] = {
          JccPatterns::rel8(Opcode::JG, 0x7F),   // 0x7F      cb
          JccPatterns::rel32(Opcode::JG, 0x8F),  // 0x0F 0x8F cd
      };

      /// ============================================================================
      /// JGE — Jump if Greater or Equal (SF=OF)
      /// ============================================================================
      table[static_cast<size_t>(Opcode::JGE)] = {
          JccPatterns::rel8(Opcode::JGE, 0x7D),   // 0x7D      cb
          JccPatterns::rel32(Opcode::JGE, 0x8D),  // 0x0F 0x8D cd
      };

      /// ============================================================================
      /// CALL — Call Procedure
      /// ============================================================================
      table[static_cast<size_t>(Opcode::CALL)] = {
          CallPatterns::rel32(),  // 0xE8  cd
          CallPatterns::rm64(),   // 0xFF /2
      };

      /// ============================================================================
      /// RET — Return from Procedure
      /// ============================================================================
      table[static_cast<size_t>(Opcode::RET)] = {
          RetPatterns::ret(),        // 0xC3
          RetPatterns::ret_imm16(),  // 0xC2  iw
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

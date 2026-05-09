/// @file InstructionEncoder.h
/// @brief x86-64 instruction encoder driven by OpcodeTable
#pragma once

#include <cstdint>
#include <vector>

#include "ast/Instruction.h"
#include "common/CompilerError.h"
#include "ir/Program.h"
#include "opcodes/OpcodePattern.h"

/// @brief Encodes IR instructions into x86-64 machine code.
///
/// The encoder:
/// - Matches instructions against OpcodeTable patterns
/// - Encodes operands into ModRM/SIB/immediates
/// - Produces final instruction bytes
///
/// This component is used by both:
/// - SizeResolver (size estimation)
/// - ElfWriter
class InstructionEncoder {
 public:
  /// @brief Encodes a single instruction into machine code bytes
  ///
  /// @param instruction IR instruction to encode
  /// @param program Full program IR used for symbol resolution
  /// @param baseVaddr Virtual address of instruction
  ///
  /// @return Encoded machine code bytes
  std::vector<uint8_t> encode(const Instruction& instruction,
                              const Program& program, uint64_t baseVaddr);

  /// @brief Computes encoded instruction size without emitting bytes
  ///
  /// Used during layout stabilization passes by SizeResolver
  ///
  /// @param instruction IR instruction
  /// @param program Full program IR
  /// @param baseVaddr Virtual address of instruction
  ///
  /// @return Encoded instruction size in bytes
  std::size_t size(const Instruction& instruction, const Program& program,
                   uint64_t baseVaddr);

  /// @brief Returns encoder errors.
  const std::vector<CompilerError>& getErrors() const noexcept;

  /// @brief Returns true if encoding produced errors.
  bool hasErrors() const noexcept;

 private:
  /// Collected encoding errors
  std::vector<CompilerError> errors;

  /// @brief Adds encoding error
  ///
  /// @param msg Error message
  /// @param span Source code location
  void addError(const std::string& msg, SourceSpan span = {});

  /// @brief Finds matching opcode pattern for instruction
  ///
  /// Returns the first OpcodePattern whose operand constraints
  /// match the instruction operands
  ///
  /// @param instruction Instruction to match
  ///
  /// @return Matching opcode pattern or nullptr
  const OpcodePattern* matchPattern(const Instruction& instruction) const;

  /// @brief Checks whether operand satisfies encoding constraint
  ///
  /// @param op Operand to validate
  /// @param constraint Operand constraint from OpcodePattern
  ///
  /// @return True if operand matches constraint
  bool matchOperand(const Operand& op,
                    const OperandConstraint& constraint) const;

  /// @brief Emits machine code bytes for matched opcode pattern
  ///
  /// @param pattern Matched opcode pattern
  /// @param instruction Instruction being encoded
  /// @param program Full program IR
  /// @param baseVaddr Virtual address of instruction
  ///
  /// @return Encoded instruction bytes
  std::vector<uint8_t> emitPattern(const OpcodePattern& pattern,
                                   const Instruction& instruction,
                                   const Program& program, uint64_t baseVaddr);

  // ─────────────────────────────────────────────────────────────
  // x86-64 byte encoding helpers
  // ─────────────────────────────────────────────────────────────

  /// @brief Builds REX prefix byte.
  static uint8_t rexByte(bool W, bool R = false, bool X = false,
                         bool B = false);

  /// @brief Builds ModRM byte.
  static uint8_t modRM(uint8_t mod, uint8_t reg, uint8_t rm);

  /// @brief Builds SIB byte.
  static uint8_t sibByte(uint8_t scale, uint8_t index, uint8_t base);

  /// @brief Emits 8-bit immediate value.
  static void emitImm8(std::vector<uint8_t>& buffer, uint8_t v);

  /// @brief Emits 16-bit immediate value.
  static void emitImm16(std::vector<uint8_t>& buffer, uint16_t v);

  /// @brief Emits 32-bit immediate value.
  static void emitImm32(std::vector<uint8_t>& buffer, uint32_t v);

  /// @brief Emits 64-bit immediate value.
  static void emitImm64(std::vector<uint8_t>& buffer, uint64_t v);

  /// @brief Encodes memory operand into ModRM/SIB/displacement bytes
  ///
  /// @param buffer Output byte buffer
  /// @param regField ModRM reg field value
  /// @param mem Memory operand
  /// @param symbols Symbol table used for label resolution
  void emitMemOperand(std::vector<uint8_t>& buffer, uint8_t regField,
                      const MemoryOperand& mem, const SymbolTable& symbols);

  /// @brief Resolves immediate operand into integer value
  ///
  /// Supports:
  /// - Numeric immediates
  /// - Labels
  /// - EQU constants
  ///
  /// @param op Operand to resolve.
  /// @param program Full program IR
  ///
  /// @return Resolved immediate value
  int64_t resolveImm(const Operand& op, const Program& program);

  /// @brief Computes rel32 displacement for relative instructions
  ///
  /// Formula:
  /// target - (instruction_address + instruction_size)
  ///
  /// @param target Target address
  /// @param instrVaddr Instruction virtual address
  /// @param instrSize Final encoded instruction size
  ///
  /// @return Signed 32-bit relative displacement
  int32_t calcRel32(uint64_t target, uint64_t instrVaddr,
                    std::size_t instrSize);
};

/// @file ElfWriter.h
/// @brief Defines the ELF executable writer.

///
/// Runs after Layout — section.virtualAddr and section.fileOffset
/// must already be correct.
///
/// Responsibilities:
/// - Emit ELF header
/// - Emit Program headers (one PT_LOAD per section)
/// - Emit section bytes using InstructionEncoder
#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "common/CompilerError.h"
#include "encoder/InstructionEncoder.h"
#include "ir/Program.h"

/// @brief Produces a Linux x86-64 ELF executable from the final IR.
///
/// The ElfWriter is the final backend stage of the assembler pipeline
///
/// Responsibilities:
/// - Emit ELF header
/// - Emit program headers (PT_LOAD segments)
/// - Encode instructions into machine code
/// - Produce the final executable binary
///
/// This stage runs after:
/// - SizeResolver
/// - Layout
///
/// Required invariants:
/// - Section sizes must already be resolved
/// - Section virtual addresses must already be assigned
/// - Section file offsets must already be assigned
class ElfWriter {
 public:
  /// @brief Creates a new ELF writer instance
  ///
  /// @param program Finalized program IR
  /// @param encoder Instruction encoder used for machine code emission
  explicit ElfWriter(Program& program, InstructionEncoder& encoder);

  /// @brief Writes ELF executable to disk
  ///
  /// @param path Output file path
  void write(const std::filesystem::path& path);

  /// @brief Returns errors produced during ELF generation
  const std::vector<CompilerError>& getErrors() const noexcept;

  /// @brief Returns true if ELF generation produced errors
  bool hasErrors() const noexcept;

 private:
  /// Program IR being emitted
  Program& program;

  /// Instruction encoder used for instruction emission
  InstructionEncoder& encoder;

  /// Collected writer errors
  std::vector<CompilerError> errors;

 private:
  /// @brief Builds the complete ELF binary in memory
  ///
  /// @return ELF binary bytes
  std::vector<uint8_t> buildElf();

  /// @brief Adds writer error
  ///
  /// @param span Source location
  /// @param msg Error message
  void addError(const SourceSpan& span, const std::string& msg);

  /// @brief Emits ELF header
  ///
  /// @param out Output byte buffer
  /// @param entryVaddr Program entry virtual address
  /// @param numSegments Number of PT_LOAD segments
  void emitElfHeader(std::vector<uint8_t>& out, uint64_t entryVaddr,
                     uint16_t numSegments);

  /// @brief Emits ELF program header for one section
  ///
  /// Each section is emitted as a PT_LOAD segment
  ///
  /// @param out Output byte buffer
  /// @param section Section metadata
  /// @param flags ELF segment permission flags
  void emitProgramHeader(std::vector<uint8_t>& out, const Section& section,
                         uint32_t flags);

  /// @brief Emits raw bytes of one section
  ///
  /// Instructions are encoded using InstructionEncoder
  ///
  /// @param out Output byte buffer
  /// @param section Section to emit
  void emitSection(std::vector<uint8_t>& out, const Section& section);

  /// @brief Emits bytes for a data directive
  ///
  /// @param out Output byte buffer
  /// @param dir Data directive to emit
  void emitDataDirective(std::vector<uint8_t>& out, const DataDirective& dir);

  /// @brief Finds virtual address of program entry point
  ///
  /// Searches for the `_start` symbol
  ///
  /// @return Entry point virtual address or std::nullopt
  std::optional<uint64_t> findEntryPoint() const;

  /// @brief Returns ELF PT_LOAD flags for section type
  ///
  /// Mapping:
  /// - .text → PF_R | PF_X (read + execute)
  /// - .data → PF_R | PF_W (read + write)
  /// - .bss  → PF_R | PF_W (read + write)
  ///
  /// @param type Section type
  ///
  /// @return ELF segment flags
  static uint32_t sectionFlags(SectionDirectiveType type);

  // =============================================================
  // Little-endian write helpers
  // =============================================================

  /// @brief Writes 16-bit little-endian value
  static void write16(std::vector<uint8_t>& out, uint16_t v);

  /// @brief Writes 32-bit little-endian value
  static void write32(std::vector<uint8_t>& out, uint32_t v);

  /// @brief Writes 64-bit little-endian value
  static void write64(std::vector<uint8_t>& out, uint64_t v);

  /// @brief Writes padding bytes
  ///
  /// @param out Output byte buffer
  /// @param n Number of padding bytes
  static void writePad(std::vector<uint8_t>& out, std::size_t n);
};

/// @file SizeResolver.h
/// @brief Resolves section sizes and symbol offsets
#pragma once
#include "encoder/InstructionEncoder.h"
#include "ir/Program.h"

/// The SizeResolver performs iterative layout passes over the program IR
/// until all instruction sizes and symbol offsets stabilize
///
/// This stage:
/// - Computes instruction offsets inside sections
/// - Resolves label addresses
/// - Computes final section sizes
/// - Handles variable-size instruction encodings
///
/// Instruction sizes are obtained through InstructionEncoder
///
/// Multi-pass resolution is required because some x86-64 instructions
/// may change size depending on final offsets and relative distances
class SizeResolver {
 public:
  /// @brief Creates a new SizeResolver instance
  ///
  /// @param program Program IR to resolve
  /// @param encoder Instruction encoder used for size estimation
  explicit SizeResolver(Program& program, InstructionEncoder& encoder);

  /// @brief Runs iterative layout passes until offsets stabilize
  ///
  /// Most programs converge in 1–2 passes
  ///
  /// This stage updates:
  /// - Symbol offsets
  /// - Section sizes
  /// - Statement layout information
  void resolve();

  /// @brief Returns errors produced during resolution.
  const std::vector<CompilerError>& getErrors() const noexcept;

  /// @brief Returns true if resolution produced errors.
  bool hasErrors() const noexcept;

 private:
  /// Program IR being resolved
  Program& program;

  /// Instruction encoder used for instruction size estimation
  InstructionEncoder& encoder;

  /// Collected resolution errors
  std::vector<CompilerError> errors;

  /// @brief Executes a single layout pass over all sections.
  ///
  /// @return True if any offset changed and another pass is required.
  bool pass();

  /// @brief Executes a layout pass for one section
  ///
  /// @param section Section being processed
  /// @param sectionIndex Index of section in Program
  ///
  /// @return True if any offset changed
  bool passSection(Section& section, std::size_t sectionIndex);

  /// @brief Estimates byte size of a data directive
  ///
  /// @param dir Data directive
  /// @param currentOffset Current offset inside section
  ///
  /// @return Estimated size in bytes
  std::size_t estimateDataSize(const DataDirective& dir,
                               std::size_t currentOffset);

  /// @brief Estimates size of data values inside directive
  ///
  /// @param dir Data directive
  /// @param unitSize Size of one element in bytes
  ///
  /// @return Total size in bytes
  std::size_t estimateDataValues(const DataDirective& dir,
                                 std::size_t unitSize);
};

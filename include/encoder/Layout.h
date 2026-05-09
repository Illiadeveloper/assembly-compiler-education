/// @file Layout.h
/// @brief Defines the Layout stage
#pragma once
#include "common/CompilerError.h"
#include "ir/Program.h"

/// @brief Assigns virtual addresses and file offsets to sections.
///
/// The Layout stage computes final memory and file placement
/// for all sections after size resolution is complete
///
/// This stage:
/// - Assigns file offsets
/// - Assigns virtual memory addresses
/// - Applies section/page alignment
///
/// Layout operates only on section metadata and does not
/// perform instruction encoding or symbol resolution
///
/// SizeResolver must run before this stage because all
/// section sizes must already be finalized
class Layout {
 public:
  /// @brief Creates a new Layout instance.
  ///
  /// @param program Program IR to layout.
  explicit Layout(Program& program);

  /// @brief Assigns virtual addresses and file offsets to sections
  ///
  /// Updates:
  /// - Section::fileOffset
  /// - Section::virtualAddr
  ///
  /// Uses page alignment rules suitable for ELF x86-64 binaries
  void assign();

  /// @brief Returns layout errors.
  const std::vector<CompilerError>& getErrors() const noexcept;

  /// @brief Returns true if layout produced errors.
  bool hasErrors() const noexcept;

 private:
  /// Program IR being processed
  Program& program;

  /// Collected layout errors
  std::vector<CompilerError> errors;

  /// @brief Aligns value upward to the specified alignment
  ///
  /// Example:
  /// alignUp(0x1003, 0x1000) -> 0x2000
  ///
  /// @param value Value to align.
  /// @param align Alignment boundary
  ///
  /// @return Aligned value
  static std::size_t alignUp(std::size_t value, std::size_t align);
};

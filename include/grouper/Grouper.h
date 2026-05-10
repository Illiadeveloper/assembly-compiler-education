/// @file Grouper.h
/// @brief Converts a flat list of statements into a structured IR program.
///
/// The Grouper transforms a FlatProgram into the final IR representation
/// used by later backend stages.
///
/// During grouping it:
/// - Splits statements into sections (.text, .data, .bss, ...)
/// - Builds the symbol table
/// - Processes labels and directives
///
/// The Grouper does NOT resolve section offsets, sizes, or virtual addresses.
/// Those are handled later by layout and size resolution stages.
#pragma once

#include <functional>
#include <optional>
#include <string>

#include "ast/Statement.h"
#include "common/CompilerError.h"
#include "ir/Program.h"

/// @brief Converts a flat statement list into a structured IR program
class Grouper {
 public:
  /// @brief Creates a new Grouper instance
  /// @param program Flat program produced by the parser
  explicit Grouper(FlatProgram program);

  /// @brief Groups statements into sections and builds symbol table.
  /// @return Structured IR program.
  Program group();

  /// @brief Returns all compilation errors produced during grouping
  const std::vector<CompilerError>& getErrors() const noexcept;

  /// @brief Checks whether grouping produced any errors.
  bool hasErrors() const noexcept;

 private:
  /// Flat input program produced by parser
  FlatProgram flatProgram;

  /// Resulting structured IR program
  Program result;

  /// Index of currently active section
  size_t currentSectionIndex = static_cast<size_t>(-1);

  /// Collected compilation errors
  std::vector<CompilerError> errors;

 private:
  /// @brief Adds compilation error
  /// @param span Source code location
  /// @param msg Error message
  void addError(const SourceSpan& span, const std::string& msg);

  /// @brief Processes a single statement
  void processStatement(const Statement& stmt);

  /// Handles instruction statement
  void handleInstruction(const Instruction& instruction);

  /// Handles label definition
  void handleLabel(const Label& label);

  /// Handles section directive (.text, .data, ...)
  void handleSectionDirective(const SectionDirective& directive);

  /// Handles EQU constant definition
  void handleEquDirective(const EquDirective& directive);

  /// Handles global symbol directive
  void handleGlobalDirective(const GlobalDirective& directive);

  /// Handles data definition directives
  void handleDataDirective(const DataDirective& directive);

  /// @brief Returns currently active section
  /// @return Reference to current section if exists
  std::optional<std::reference_wrapper<Section>> currentSection();

  /// @brief Returns existing section or creates a new one
  /// @param type Section type
  /// @return Reference to section
  Section& getOrCreateSection(SectionDirectiveType type);

  /// @brief Returns existing symbol or creates a new one
  /// @param name Symbol name
  /// @return Reference to symbol
  Symbol& getOrCreateSymbol(const std::string& name);
};

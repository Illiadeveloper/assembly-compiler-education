/// @file Program.h
/// @brief Intermediate Representation (IR) of an assembly program
///
/// This file defines core IR structures used by the assembler:
/// - Program (top-level container)
/// - Section (.text, .data, .bss, etc.)
/// - Symbol table (labels and constants)
///
/// It is used directly by the code generation phase to produce
/// the final binary output.
#pragma once

#include "ast/Directive.h"
#include "ast/Statement.h"

/// @brief Represents a single section in the intermediate representation.
///        Example sections: .text, .data, .bss.
struct Section {
  /// Type of section (.text, .data, .bss, etc.)
  SectionDirectiveType type;

  /// List of instructions/statements inside this section
  std::vector<Statement> items;

  /// @brief Size of section in bytes (computed during layout phase)
  uint64_t size = 0;

  /// @brief Offset of section in final binary file
  uint64_t fileOffset = 0;

  /// @brief Virtual memory address of section
  /// Example:
  ///   .text → 0x400000
  ///   .data → 0x401000
  uint64_t virtualAddr = 0;
};

enum class SymbolKind { LABEL, EQU };

struct Symbol {
  /// Kind of symbol (LABEL or EQU)
  SymbolKind kind;

  /// Index of section where symbol is defined
  size_t sectionIndex = 0;

  /// Index of statement inside section
  size_t statementIndex = 0;

  /// Offset inside section (resolved during layout phase)
  uint64_t offset = 0;

  /// Value of EQU symbol (valid only if kind == EQU)
  int64_t value = 0;

  /// Whether symbol is global
  bool isGlobal = false;
};

/// @brief Symbol table mapping names to symbols
using SymbolTable = std::unordered_map<std::string, Symbol>;

/// @brief Full intermediate representation of a program
///        Contains sections and resolved symbols
struct Program {
  /// All program sections
  std::vector<Section> sections;

  /// Global symbol table
  SymbolTable symbols;
};

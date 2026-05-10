/// @file Directive.h
/// @brief Assembler directive AST
#pragma once

#include "common/Token.h"
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

// =================== Data Directive =========================
/// @brief Kinds of data directives
enum class DataDirectiveKind {
  DB, ///< Define byte
  DW, ///< Define word (2 bytes)
  DD, ///< Define double word (4 bytes)
  DQ, ///< Define quad word (8 bytes)

  RESB, ///< Reserve bytes
  RESW, ///< Reserve words
  RESD, ///< Reserve double words
  RESQ, ///< Reserve quad words

  ALIGN
};

inline const std::unordered_map<std::string, DataDirectiveKind>
    DataDirectiveTable = {
        {"db", DataDirectiveKind::DB},       {"dw", DataDirectiveKind::DW},
        {"dd", DataDirectiveKind::DD},       {"dq", DataDirectiveKind::DQ},
        {"resb", DataDirectiveKind::RESB},   {"resw", DataDirectiveKind::RESW},
        {"resd", DataDirectiveKind::RESD},   {"resq", DataDirectiveKind::RESQ},
        {"align", DataDirectiveKind::ALIGN},
};

/// Literal value used in data directives
/// Example:
///   db 1
///   db 3.14
///   db "hello"
struct DataValue {
  std::variant<int64_t, double, std::string> value;
};

/// Data directive node (e.g. 'db', 'resb', 'align')
struct DataDirective {
  std::optional<std::string> label;
  DataDirectiveKind kind;

  /// Empty for reserve directives
  std::vector<DataValue> values;

  /// repeat count or size for res*
  int64_t count = 1;

  SourceSpan span;
};

// =================== Section =====================
/// @brief Section directive types
enum class SectionDirectiveType : uint8_t {
  DATA,
  TEXT,
  BSS,
};

inline const std::unordered_map<std::string, SectionDirectiveType>
    SectionTable = {
        {".data", SectionDirectiveType::DATA},
        {".text", SectionDirectiveType::TEXT},
        {".bss", SectionDirectiveType::BSS},

        {"data", SectionDirectiveType::DATA},
        {"text", SectionDirectiveType::TEXT},
        {"bss", SectionDirectiveType::BSS},
};


/// Section directive node
struct SectionDirective {
  SectionDirectiveType type;
  SourceSpan span;
};

// =================== Equal Directive =========================
/// 'name equ value'
struct EquDirective {
  /// Constant name
  std::string name;

  int64_t value;
  SourceSpan span;
};

// =================== Global Directive =========================
/// @brief 'global name'
struct GlobalDirective {
  std::string name; 
  SourceSpan span;
};

// =================== Directive Keywords =======================

/// @brief Directive keywords
enum class DirectiveKeyword {
  SECTION,
  GLOBAL,
  EQU,
  TIMES,
};

inline const std::unordered_map<std::string, DirectiveKeyword>
    DirectiveKeywordTable = {
        {"section", DirectiveKeyword::SECTION},
        {"global", DirectiveKeyword::GLOBAL},
        {"equ", DirectiveKeyword::EQU},
        {"times", DirectiveKeyword::TIMES},
};

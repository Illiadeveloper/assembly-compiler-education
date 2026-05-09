/// @file SizeResolver.cpp
#include "encoder/SizeResolver.h"

SizeResolver::SizeResolver(Program& program, InstructionEncoder& encoder)
    : program(program), encoder(encoder) {}

const std::vector<CompilerError>& SizeResolver::getErrors() const noexcept {
  return errors;
}

bool SizeResolver::hasErrors() const noexcept {
  return !errors.empty();
}

// ===========================================================
//  Public
// ===========================================================

void SizeResolver::resolve() {
  constexpr int MAX_PASSES = 10; // defense from infinite loop
  int passCount = 0;

  while (true) {
    if (passCount++ >= MAX_PASSES) {
      errors.push_back(CompilerError{
        .stage   = ErrorStage::SIZE_RESOLVER,
        .span    = {},
        .message = "failed to stabilize after " +
                   std::to_string(MAX_PASSES) + " passes"
      });
      return;
    }

    bool changed = pass();
    if (!changed) break; // everything has stabalized 
  }
}

// ===========================================================
//  Passes
// ===========================================================

bool SizeResolver::pass() {
  bool changed = false;

  for (std::size_t i = 0; i < program.sections.size(); ++i) {
    if (passSection(program.sections[i], i))
      changed = true;
  }

  return changed;
}

bool SizeResolver::passSection(Section& section, std::size_t sectionIndex) {
  bool changed = false;
  std::size_t offset = 0;

  for (std::size_t stmtIndex = 0; stmtIndex < section.items.size(); ++stmtIndex) {
    const Statement& stmt = section.items[stmtIndex];

    std::visit([&](const auto& node) {
      using T = std::decay_t<decltype(node)>;

      if constexpr (std::is_same_v<T, Label>) {
        // Метка — обновляем offset в SymbolTable
        auto it = program.symbols.find(node.name);
        if (it == program.symbols.end()) {
          errors.push_back(CompilerError{
            .stage   = ErrorStage::SIZE_RESOLVER,
            .span    = node.span,
            .message = "label not found in symbol table: " + node.name
          });
          return;
        }

        Symbol& sym = it->second;
        if (sym.offset != offset) {
          sym.offset = offset; // offset has changed -> need new pass
          changed = true;
        }

      } else if constexpr (std::is_same_v<T, Instruction>) {
        std::size_t instrSize = encoder.size(node, program,
                                              section.virtualAddr + offset);
        if (encoder.hasErrors()) {
          for (const auto& e : encoder.getErrors())
            errors.push_back(e);
          return;
        }

        offset += instrSize;

      } else if constexpr (std::is_same_v<T, DataDirective>) {
        if (node.label.has_value()) {
          auto it = program.symbols.find(*node.label);
          if (it != program.symbols.end()) {
            Symbol& sym = it->second;
            if (sym.offset != offset) {
              sym.offset = offset;
              changed = true;
            }
          }
        }

        offset += estimateDataSize(node, offset);
      }
      // Label, SectionDirective, EquDirective - size 0
    }, stmt);
  }

  // Обновляем size секции
  if (section.size != offset) {
    section.size = offset;
    changed = true;
  }

  return changed;
}

// ===========================================================
//  Data size estimation
// ===========================================================

std::size_t SizeResolver::estimateDataSize(const DataDirective& dir,
                                            std::size_t currentOffset) {
  switch (dir.kind) {
    case DataDirectiveKind::ALIGN: {
      if (dir.count <= 1) return 0;
      const std::size_t align = static_cast<std::size_t>(dir.count);
      return (align - (currentOffset % align)) % align;
    }
    case DataDirectiveKind::RESB: return 1 * dir.count;
    case DataDirectiveKind::RESW: return 2 * dir.count;
    case DataDirectiveKind::RESD: return 4 * dir.count;
    case DataDirectiveKind::RESQ: return 8 * dir.count;
    case DataDirectiveKind::DB: return estimateDataValues(dir, 1);
    case DataDirectiveKind::DW: return estimateDataValues(dir, 2);
    case DataDirectiveKind::DD: return estimateDataValues(dir, 4);
    case DataDirectiveKind::DQ: return estimateDataValues(dir, 8);
  }
  return 0;
}

std::size_t SizeResolver::estimateDataValues(const DataDirective& dir,
                                              std::size_t unitSize) {
  std::size_t onePass = 0;
  for (const auto& val : dir.values) {
    onePass += std::visit([&](const auto& v) -> std::size_t {
      using T = std::decay_t<decltype(v)>;
      if constexpr (std::is_same_v<T, std::string>)
        return unitSize * v.size();
      else
        return unitSize;
    }, val.value);
  }
  return onePass * std::max<std::size_t>(1, dir.count);
}

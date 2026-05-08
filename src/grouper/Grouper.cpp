#include "grouper/Grouper.h"

#include <functional>
#include <optional>

Grouper::Grouper(FlatProgram program) : flatProgram(std::move(program)) {}

Program Grouper::group() {
  for (const auto& stmt : flatProgram) {
    processStatement(stmt);
  }

  return std::move(result);
}

void Grouper::processStatement(const Statement& stmt) {
  std::visit(
      [this](const auto& node) {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, Instruction>) {
          handleInstruction(node);
        } else if constexpr (std::is_same_v<T, Label>) {
          handleLabel(node);
        } else if constexpr (std::is_same_v<T, SectionDirective>) {
          handleSectionDirective(node);
        } else if constexpr (std::is_same_v<T, EquDirective>) {
          handleEquDirective(node);
        } else if constexpr (std::is_same_v<T, GlobalDirective>) {
          handleGlobalDirective(node);
        } else if constexpr (std::is_same_v<T, DataDirective>) {
          handleDataDirective(node);
        }
      },
      stmt);
}

void Grouper::handleSectionDirective(const SectionDirective& directive) {
  Section& sec = getOrCreateSection(directive.type);

  currentSectionIndex = &sec - result.sections.data();
}

std::optional<std::reference_wrapper<Section>> Grouper::currentSection() {
  if (currentSectionIndex == static_cast<size_t>(-1)) {
    return std::nullopt;
  }
  return result.sections[currentSectionIndex];
}

Section& Grouper::getOrCreateSection(SectionDirectiveType type) {
  for (auto& section : result.sections) {
    if (section.type == type) {
      return section;
    }
  }

  result.sections.push_back(Section{.type = type});
  return result.sections.back();
}

void Grouper::handleInstruction(const Instruction& instruction) {
  auto section = currentSection();
  if (!section) {
    addError(instruction.span, "Instruction outside of any section");
    return;
  }
  section->get().items.push_back(instruction);
}

void Grouper::handleLabel(const Label& label) {
  auto section = currentSection();
  if (!section) {
    addError(label.span, "Label outside of any section");
    return;
  }

  auto& symbol = getOrCreateSymbol(label.name);
  size_t index = section->get().items.size();
  section->get().items.push_back(label);

  symbol.kind = SymbolKind::LABEL;
  symbol.sectionIndex = currentSectionIndex;
  symbol.statementIndex = index;
}

void Grouper::handleEquDirective(const EquDirective& directive) {
  auto& sym = getOrCreateSymbol(directive.name);
  sym.kind = SymbolKind::EQU;
  sym.value = directive.value;
}

void Grouper::handleGlobalDirective(const GlobalDirective& directive) {
  auto& sym = getOrCreateSymbol(directive.name);
  sym.isGlobal = true;
}

void Grouper::handleDataDirective(const DataDirective& directive) {
  auto section = currentSection();
  if (!section) {
    addError(directive.span, "Data Directive outside of any section");
    return;
  }

  section->get().items.push_back(directive);

  if (directive.label.has_value()) {
    auto& symbol = getOrCreateSymbol(*directive.label);
    symbol.kind = SymbolKind::LABEL;
    symbol.sectionIndex = currentSectionIndex;
    symbol.statementIndex = section->get().items.size() - 1;
  }
}

Symbol& Grouper::getOrCreateSymbol(const std::string& name) {
  return result.symbols[name];
}

void Grouper::addError(const SourceSpan& span, const std::string& msg) {
  errors.push_back({ErrorStage::GROUPER, span, msg});
}

const std::vector<CompilerError>& Grouper::getErrors() const noexcept {
  return errors;
}
bool Grouper::hasErrors() const noexcept { return errors.size() > 0; }

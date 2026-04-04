#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace flatpr {

// helper: indentation
inline std::string indent(int n) { return std::string(n * 2, ' '); }

// helper: if T is enum -> print numeric, else print type name
template <typename T> std::string enumOrTypeName(const T &v) {
  if constexpr (std::is_enum_v<std::decay_t<T>>) {
    return std::to_string(
        static_cast<std::underlying_type_t<std::decay_t<T>>>(v));
  } else {
    return std::string(typeid(v).name());
  }
}

// Format SourceSpan
inline std::string spanToString(const SourceSpan &s) {
  std::ostringstream oss;
  oss << "line=" << s.line << ", col=" << s.column << ", len=" << s.length;
  return oss.str();
}

// Forward declarations
void printOperand(const Operand &op, std::ostream &os, int level = 0);
void printMemoryOperand(const MemoryOperand &m, std::ostream &os,
                        int level = 0);

// Print an Operand (variant)
void printOperand(const Operand &op, std::ostream &os, int level) {
  os << indent(level);
  os << "- kind = ";
  switch (op.kind) {
  case OperandKind::REG:
    os << "REG";
    break;
  case OperandKind::IMM:
    os << "IMM";
    break;
  case OperandKind::MEM:
    os << "MEM";
    break;
  case OperandKind::LABEL:
    os << "LABEL";
    break;
  default:
    os << "UNKNOWN(" << static_cast<int>(op.kind) << ")";
    break;
  }
  os << ", size = " << enumOrTypeName(op.size) << "\n";

  os << indent(level) << "  value: ";
  if (op.isReg()) {
    // Register may be enum or class — print enum underlying or type name
    try {
      const Register r = op.getReg();
      os << enumOrTypeName(r) << "\n";
    } catch (...) {
      os << "<cannot get reg>\n";
    }
  } else if (op.isImm()) {
    try {
      os << op.getImm() << "\n";
    } catch (...) {
      os << "<cannot get imm>\n";
    }
  } else if (op.isMem()) {
    os << "\n";
    try {
      printMemoryOperand(op.getMem(), os, level + 2);
    } catch (...) {
      os << indent(level + 2) << "<cannot get mem>\n";
    }
  } else if (op.isLabel()) {
    try {
      os << op.getLabel() << "\n";
    } catch (...) {
      os << "<cannot get label>\n";
    }
  } else {
    os << "<unknown operand>\n";
  }
}

void printMemoryOperand(const MemoryOperand &m, std::ostream &os, int level) {
  os << indent(level) << "MemoryOperand:\n";
  os << indent(level) << "  base: ";
  if (m.base.has_value())
    os << enumOrTypeName(*m.base) << "\n";
  else
    os << "none\n";
  os << indent(level) << "  index: ";
  if (m.index.has_value())
    os << enumOrTypeName(*m.index) << "\n";
  else
    os << "none\n";
  os << indent(level) << "  scale: " << m.scale << "\n";
  os << indent(level) << "  displacement: " << m.displacement << "\n";
  os << indent(level) << "  size: " << enumOrTypeName(m.size) << "\n";

  if (!m.symbols.empty()) {
    os << indent(level) << "  symbols:\n";
    for (const auto &s : m.symbols) {
      os << indent(level + 1) << (s.sign >= 0 ? "+ " : "- ") << s.name << "\n";
    }
  }
}

// Print Instruction
void printInstruction(const Instruction &ins, std::ostream &os, int level = 0) {
  os << indent(level) << "Instruction:\n";
  // opcode: can't assume printable -> show typeid name or enum underlying
  os << indent(level + 1) << "opcode: " << enumOrTypeName(ins.opcode) << "\n";

  os << indent(level + 1) << "operands (" << ins.operands.size() << "):\n";
  for (const auto &op : ins.operands) {
    printOperand(op, os, level + 2);
  }
  // SourceSpan: print line/column/length
  os << indent(level + 1) << "span: " << spanToString(ins.span) << "\n";
}

// Print DataDirective
void printDataDirective(const DataDirective &d, std::ostream &os,
                        int level = 0) {
  os << indent(level) << "DataDirective:\n";
  os << indent(level + 1) << "kind: " << enumOrTypeName(d.kind) << "\n";
  if (d.label.has_value()) {
    os << indent(level + 1) << "label: " << *d.label << "\n";
  }
  if (d.count != 1) {
    os << indent(level + 1) << "count: " << d.count << "\n";
  }
  os << indent(level + 1) << "values (" << d.values.size() << "):\n";
  for (const auto &v : d.values) {
    os << indent(level + 2);
    std::visit(
        [&os](auto &&val) {
          using T = std::decay_t<decltype(val)>;
          if constexpr (std::is_same_v<T, int64_t>) {
            os << val << " (int)\n";
          } else if constexpr (std::is_same_v<T, double>) {
            os << val << " (double)\n";
          } else if constexpr (std::is_same_v<T, std::string>) {
            os << "\"" << val << "\" (string)\n";
          } else {
            os << "<unknown value>\n";
          }
        },
        v.value);
  }
  os << indent(level + 1) << "span: " << spanToString(d.span) << "\n";
}

// Print other simple directives
void printSectionDirective(const SectionDirective &s, std::ostream &os,
                           int level = 0) {
  os << indent(level) << "SectionDirective: type=" << enumOrTypeName(s.type)
     << ", span=(" << spanToString(s.span) << ")\n";
}
void printEquDirective(const EquDirective &e, std::ostream &os, int level = 0) {
  os << indent(level) << "EquDirective: " << e.name << " = " << e.value
     << ", span=(" << spanToString(e.span) << ")\n";
}
void printGlobalDirective(const GlobalDirective &g, std::ostream &os,
                          int level = 0) {
  os << indent(level) << "GlobalDirective: " << g.name << ", span=("
     << spanToString(g.span) << ")\n";
}
void printLabel(const Label &l, std::ostream &os, int level = 0) {
  os << indent(level) << "Label: " << l.name << ", span=("
     << spanToString(l.span) << ")\n";
}

// Print a Statement (variant)
void printStatement(const Statement &st, std::ostream &os, int level = 0) {
  std::visit(
      [&os, level](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Instruction>) {
          printInstruction(arg, os, level);
        } else if constexpr (std::is_same_v<T, Label>) {
          printLabel(arg, os, level);
        } else if constexpr (std::is_same_v<T, SectionDirective>) {
          printSectionDirective(arg, os, level);
        } else if constexpr (std::is_same_v<T, EquDirective>) {
          printEquDirective(arg, os, level);
        } else if constexpr (std::is_same_v<T, GlobalDirective>) {
          printGlobalDirective(arg, os, level);
        } else if constexpr (std::is_same_v<T, DataDirective>) {
          printDataDirective(arg, os, level);
        } else {
          os << indent(level) << "<unknown statement type>\n";
        }
      },
      st);
}

// Print whole program
inline void printFlatProgram(const FlatProgram &program,
                             std::ostream &os = std::cout) {
  os << "FlatProgram: " << program.size() << " statements\n";
  size_t idx = 0;
  for (const auto &st : program) {
    os << "[" << std::setw(3) << idx++ << "] ";
    printStatement(st, os, 0);
  }
}
} // namespace flatpr

// --------------------- test helpers ---------------------
void printTokensWithSpan(const std::vector<Token> &tokens) {
  for (const auto &t : tokens) {
    std::cout << "TYPE: " << (int)t.type << "\tVALUE: '";
    if (auto *v = std::get_if<int64_t>(&t.value)) {
      std::cout << *v;
    } else if (auto *s = std::get_if<std::string>(&t.value)) {
      std::cout << *s;
    } else {
      std::cout << "<empty>";
    }
    std::cout << "'\tSPAN: line=" << t.span.line << ",col=" << t.span.column
              << ",len=" << t.span.length << "\n";
  }
}

void test(const std::string &src) {
  std::cout << "=== SOURCE ===\n" << src << "\n";
  Lexer lex(src);
  auto tokens = lex.tokenize();

  std::cout << "--- TOKENS ---\n";
  printTokensWithSpan(tokens);
  std::cout << "\n";
}

int main() {
  std::string src = R"(

section .data

msg     db "Hello world", 0
numbers dq 1, 2, 3, 4
small   db 10, 20, 30
wordv   dw 1000
dwordv  dd 123456

times_test times 4 db 7

section .bss

buffer  resb 64
array   resq 8

section .text
global _start

_start:
    mov rax, 1
    mov rdi, 2
    add rax, rdi
    mov rbx, numbers
    mov rcx, [numbers + 8]

)";

  Lexer lex(src);
  auto tokens = lex.tokenize();

  Parser par(tokens);
  auto program = par.parse();

  if (par.hasErrors()) {
    for (auto &e : par.getErrors()) {
      std::cout << "ERROR: " << e.message << "\n";
    }
  }

  flatpr::printFlatProgram(program);

  return 0;
}


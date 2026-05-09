#include "encoder/InstructionEncoder.h"

#include <cstdint>

#include "common/OperandTypes.h"
#include "opcodes/OpcodePattern.h"
#include "opcodes/OpcodeTable.h"

// ===========================================================
//  Public API
// ===========================================================

std::vector<uint8_t> InstructionEncoder::encode(const Instruction& instruction,
                                                const Program& program,
                                                uint64_t baseVaddr) {
  const OpcodePattern* pattern = matchPattern(instruction);

  if (!pattern) {
    addError("No matching pattern for instruction", instruction.span);
    return {};
  }

  return emitPattern(*pattern, instruction, program, baseVaddr);
}

std::size_t InstructionEncoder::size(const Instruction& instruction,
                                     const Program& program,
                                     uint64_t baseVaddr) {
  return encode(instruction, program, baseVaddr).size();
}

// ===========================================================
//  Error
// ===========================================================

void InstructionEncoder::addError(const std::string& msg, SourceSpan span) {
  errors.push_back(CompilerError{
      .stage = ErrorStage::ENCODER, .span = span, .message = msg});
}

const std::vector<CompilerError>& InstructionEncoder::getErrors()
    const noexcept {
  return errors;
}

bool InstructionEncoder::hasErrors() const noexcept { return !errors.empty(); }

// ===========================================================
//  Pattern matching
// ===========================================================

const OpcodePattern* InstructionEncoder::matchPattern(
    const Instruction& instruction) const {
  const auto& patterns = OpcodeTable[static_cast<size_t>(instruction.opcode)];

  for (const auto& pattern : patterns) {
    // количество операндов должно совпадать
    if (pattern.operands.size() != instruction.operands.size()) continue;

    bool ok = true;
    for (std::size_t i = 0; i < instruction.operands.size(); ++i) {
      if (!matchOperand(instruction.operands[i], pattern.operands[i])) {
        ok = false;
        break;
      }
    }
    if (ok) return &pattern;
  }

  return nullptr;
}

bool InstructionEncoder::matchOperand(
    const Operand& operand, const OperandConstraint& constraint) const {
  // проверяем kind
  OperandKindMask kindBit = OK_NONE;
  switch (operand.kind) {
    case OperandKind::REG:
      kindBit = OK_REG;
      break;
    case OperandKind::MEM:
      kindBit = OK_MEM;
      break;
    case OperandKind::IMM:
      kindBit = OK_IMM;
      break;
    case OperandKind::LABEL:
      kindBit = OK_LABEL;
      break;
  }

  if (operand.isLabel() && (constraint.allowed & OK_IMM)) kindBit = OK_IMM;

  if (!(constraint.allowed & kindBit)) {
    // std::cout << "Not allowed... " << std::endl;
    // std::cout << (int)operand.kind << std::endl;
    return false;
  };

  // проверяем size (ANY = любой)
  if (constraint.size != OperandSize::ANY && operand.size != OperandSize::ANY) {
    if (operand.size != constraint.size) return false;
  }

  // проверяем exactReg если задан
  if (constraint.exactReg.has_value()) {
    if (!operand.isReg() || operand.getReg() != *constraint.exactReg)
      return false;
  }

  return true;
}

// ===========================================================
//  Pattern emission
// ===========================================================

std::vector<uint8_t> InstructionEncoder::emitPattern(
    const OpcodePattern& pattern, const Instruction& instruction,
    const Program& program, uint64_t baseVaddr) {
  std::vector<uint8_t> buffer;

  // 1. REX prefix 
  bool need64 = false;
  for (const auto& operand : instruction.operands) {
    if (operand.isReg() &&
        getRegisterInfo(operand.getReg()).size == OperandSize::B64) {
      need64 = true;
      break;
    }
  }
  if (need64) buffer.push_back(rexByte(true));

  // 2. Base opcode bytes
  for (uint8_t byte : pattern.baseBytes) buffer.push_back(byte);

  // 3. regInOpcode 
  if (pattern.regInOpcode) {
    for (const auto& operand : instruction.operands) {
      if (operand.isReg()) {
        buffer.back() += getRegisterInfo(operand.getReg()).code;
        break;
      }
    }
  }

  // 4. ModRM
  if (pattern.hasModRM) {
    const Operand* rmOp = nullptr;   // Memory or register
    const Operand* regOp = nullptr;  // Only register

    for (std::size_t i = 0; i < pattern.operands.size(); ++i) {
      const auto& constraint = pattern.operands[i];
      const auto& operand = instruction.operands[i];

      if (constraint.allowed & OK_IMM) continue;  // immediate — not ModRM

      if ((constraint.allowed & OK_MEM) ||
          (constraint.allowed & OK_REG && constraint.allowed & OK_MEM)) {
        rmOp = &operand;
      } else if (constraint.allowed == OK_REG) {
        regOp = &operand;
      }
    }

    uint8_t regField = pattern.modrm_reg.value_or(0);
    if (regOp && regOp->isReg())
      regField = getRegisterInfo(regOp->getReg()).code;

    if (rmOp && rmOp->isMem()) {
      emitMemOperand(buffer, regField, rmOp->getMem(), program.symbols);
    } else if (rmOp && rmOp->isReg()) {
      buffer.push_back(
          modRM(0b11, regField, getRegisterInfo(rmOp->getReg()).code));
    }
  }

  // 5. Extra encoding (immediate / relative)
  const Operand* immOp = nullptr;
  for (const auto& op : instruction.operands) {
    if (op.isImm() || op.isLabel()) {
      immOp = &op;
      break;
    }
  }

  if (immOp) {
    switch (pattern.extra) {
      case ExtraEncoding::NONE:
        break;

      case ExtraEncoding::IMM8:
      case ExtraEncoding::IMM8_SIGNED:
        emitImm8(buffer, static_cast<uint8_t>(resolveImm(*immOp, program)));
        break;

      case ExtraEncoding::IMM16:
        emitImm16(buffer, static_cast<uint16_t>(resolveImm(*immOp, program)));
        break;

      case ExtraEncoding::IMM32:
        emitImm32(buffer, static_cast<uint32_t>(resolveImm(*immOp, program)));
        break;

      case ExtraEncoding::IMM64:
        emitImm64(buffer, static_cast<uint32_t>(resolveImm(*immOp, program)));
        break;

      case ExtraEncoding::REL8: {
        uint64_t target = static_cast<uint64_t>(resolveImm(*immOp, program));
        int64_t rel = static_cast<int64_t>(target) -
                      static_cast<int64_t>(baseVaddr + buffer.size() + 1);
        emitImm8(buffer, static_cast<uint8_t>(rel));
        break;
      }

      case ExtraEncoding::REL32: {
        uint64_t target = static_cast<uint64_t>(resolveImm(*immOp, program));
        int32_t rel = calcRel32(target, baseVaddr, buffer.size() + 4);
        emitImm32(buffer, static_cast<uint32_t>(rel));
        break;
      }
    }
  }

  return buffer;
}
// ===========================================================
//  Building blocks
// ===========================================================
uint8_t InstructionEncoder::rexByte(bool W, bool R, bool X, bool B) {
  return static_cast<uint8_t>(0x40 | (W ? 0x08 : 0) | (R ? 0x04 : 0) |
                              (X ? 0x02 : 0) | (B ? 0x01 : 0));
}

uint8_t InstructionEncoder::modRM(uint8_t mod, uint8_t reg, uint8_t rm) {
  return static_cast<uint8_t>((mod << 6) | ((reg & 0x7) << 3) | (rm & 0x7));
}

uint8_t InstructionEncoder::sibByte(uint8_t scale, uint8_t index,
                                    uint8_t base) {
  uint8_t ss = (scale == 8) ? 3 : (scale == 4) ? 2 : (scale == 2) ? 1 : 0;
  return static_cast<uint8_t>((ss << 6) | ((index & 0x7) << 3) | (base & 0x7));
}

void InstructionEncoder::emitImm8(std::vector<uint8_t>& buffer, uint8_t v) {
  buffer.push_back(v);
}

void InstructionEncoder::emitImm16(std::vector<uint8_t>& buffer, uint16_t v) {
  buffer.push_back(v & 0xFF);
  buffer.push_back(v >> 8);
}
void InstructionEncoder::emitImm32(std::vector<uint8_t>& buffer, uint32_t v) {
  for (int i = 0; i < 4; ++i) buffer.push_back((v >> (i * 8)) & 0xFF);
}
void InstructionEncoder::emitImm64(std::vector<uint8_t>& buffer, uint64_t v) {
  for (int i = 0; i < 8; ++i) buffer.push_back((v >> (i * 8)) & 0xFF);
}

void InstructionEncoder::emitMemOperand(std::vector<uint8_t>& buffer,
                                        uint8_t regField,
                                        const MemoryOperand& mem,
                                        const SymbolTable& symbols) {
  int64_t disp = mem.displacement;
  for (const auto& term : mem.symbols) {
    auto it = symbols.find(term.name);
    if (it == symbols.end()) {
      addError("Undefined symbol: " + term.name);
      return;
    }

    const Symbol& sym = it->second;
    int64_t symValue = 0;

    if (sym.kind == SymbolKind::EQU) {
      symValue = sym.value;
    } else {
      symValue = static_cast<int64_t>(sym.offset);
    }

    disp += term.sign * symValue;
  }

  const uint8_t baseCode = mem.base ? getRegisterInfo(*mem.base).code : 0;
  const uint8_t indexCode = mem.index ? getRegisterInfo(*mem.index).code : 4;
  const bool hasIndex = mem.index.has_value();

  uint8_t mod = 0b00;
  if (disp != 0) {
    mod = (disp >= -128 && disp <= 127) ? 0b01 : 0b10;
  }

  if (hasIndex) {
    buffer.push_back(modRM(mod, regField, 4));  /// rm=4 -> SIB follows
    buffer.push_back(sibByte(mem.scale, indexCode, baseCode));
  } else {
    buffer.push_back(modRM(mod, regField, baseCode));
  }

  if (mod == 0b01) emitImm8(buffer, static_cast<uint8_t>(disp));
  if (mod == 0b10) emitImm32(buffer, static_cast<uint32_t>(disp));
}

int64_t InstructionEncoder::resolveImm(const Operand& op,

                                       const Program& program) {
  if (op.isImm()) return op.getImm();
  if (op.isLabel()) {
    auto it = program.symbols.find(op.getLabel());
    if (it == program.symbols.end()) {
      addError("Undefined label: " + op.getLabel(), op.span);
      return 0;
    }
    const Symbol& sym = it->second;

    if (sym.kind == SymbolKind::EQU) return sym.value;

    uint64_t sectionVaddr = program.sections[sym.sectionIndex].virtualAddr;
    return static_cast<int64_t>(sectionVaddr + sym.offset);
  }
  addError("Operand is not imm or label", op.span);
  return 0;
}

int32_t InstructionEncoder::calcRel32(uint64_t target, uint64_t instrVaddr,
                                      std::size_t instrSize) {
  int64_t rel = static_cast<int64_t>(target) -
                static_cast<int64_t>(instrVaddr + instrSize);
  if (rel < INT32_MIN || rel > INT32_MAX) {
    addError("Relative jump out of range");
    return 0;
  }
  return static_cast<int32_t>(rel);
}

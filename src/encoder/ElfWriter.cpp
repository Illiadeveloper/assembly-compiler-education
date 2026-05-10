/// @file ElfWriter.cpp
#include "encoder/ElfWriter.h"

#include <elf.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>

#include "common/CompilerError.h"

// == ELF constants ==========================================

static constexpr uint16_t ELF_HEADER_SIZE = 64;
static constexpr uint16_t PHDR_SIZE = 56;
static constexpr uint64_t PAGE_SIZE = 0x1000;

// ===========================================================
//  Public
// ===========================================================

ElfWriter::ElfWriter(Program& program, InstructionEncoder& encoder)
    : program(program), encoder(encoder) {}

const std::vector<CompilerError>& ElfWriter::getErrors() const noexcept {
  return errors;
}

bool ElfWriter::hasErrors() const noexcept { return !errors.empty(); }

void ElfWriter::addError(const SourceSpan& span, const std::string& msg) {
  errors.push_back(CompilerError{
      .stage = ErrorStage::ELF_WRITER, .span = span, .message = msg});
}

void ElfWriter::write(const std::filesystem::path& path) {
  auto bytes = buildElf();
  if (hasErrors()) return;

  std::ofstream file(path, std::ios::binary);
  if (!file) {
    addError({}, "ElfWriter: cannot open output file: " + path.string());
    return;
  }

  file.write(reinterpret_cast<const char*>(bytes.data()),
             static_cast<std::streamsize>(bytes.size()));
}

// ===========================================================
//  Build ELF
// ===========================================================

std::vector<uint8_t> ElfWriter::buildElf() {
  std::vector<uint8_t> out;

  std::vector<Section*> sections;
  for (auto& sec : program.sections)
    if (sec.size > 0) sections.push_back(&sec);

  const std::optional<uint64_t> entryVaddr = findEntryPoint();
  if (!entryVaddr) {
    addError({}, "ElfWriter: _start has invalid section index");
    return {};
  }

  // 1. ELF Header
  emitElfHeader(out, *entryVaddr, static_cast<uint16_t>(sections.size()));

  // 2. Program Headers 
  for (const auto* sec : sections)
    emitProgramHeader(out, *sec, sectionFlags(sec->type));

  // 3. section bytes
  for (const auto* sec : sections) {
    if (sec->type == SectionDirectiveType::BSS) continue;

    // Padding to fileOffset
    while (out.size() < sec->fileOffset) out.push_back(0x00);

    emitSection(out, *sec);
  }

  return out;
}

// ===========================================================
//  ELF Header
// ===========================================================


void ElfWriter::emitElfHeader(std::vector<uint8_t>& out, uint64_t entryVaddr,
                              uint16_t numSegments) {
  Elf64_Ehdr header{};  

  // Magic + ident
  header.e_ident[EI_MAG0] = ELFMAG0;  // 0x7F
  header.e_ident[EI_MAG1] = ELFMAG1;  // 'E'
  header.e_ident[EI_MAG2] = ELFMAG2;  // 'L'
  header.e_ident[EI_MAG3] = ELFMAG3;  // 'F'
  header.e_ident[EI_CLASS] = ELFCLASS64;
  header.e_ident[EI_DATA] = ELFDATA2LSB;
  header.e_ident[EI_VERSION] = EV_CURRENT;
  header.e_ident[EI_OSABI] = ELFOSABI_NONE;

  header.e_type = ET_EXEC;
  header.e_machine = EM_X86_64;
  header.e_version = EV_CURRENT;
  header.e_entry = entryVaddr;
  header.e_phoff = sizeof(Elf64_Ehdr);
  header.e_shoff = 0;
  header.e_flags = 0;
  header.e_ehsize = sizeof(Elf64_Ehdr);
  header.e_phentsize = sizeof(Elf64_Phdr);
  header.e_phnum = numSegments;
  // header.e_shentsize = sizeof(Elf64_Shdr);
  header.e_shnum = 0;
  header.e_shstrndx = 0;

  const auto* ptr = reinterpret_cast<const uint8_t*>(&header);
  out.insert(out.end(), ptr, ptr + sizeof(header));
}

// ===========================================================
//  Program Header
// ===========================================================

void ElfWriter::emitProgramHeader(std::vector<uint8_t>& out,
                                  const Section& section, uint32_t flags) {
  Elf64_Phdr phdr{};

  phdr.p_type = PT_LOAD;
  phdr.p_flags = flags;
  phdr.p_offset = section.fileOffset;
  phdr.p_vaddr = section.virtualAddr;
  phdr.p_paddr = section.virtualAddr;
  phdr.p_filesz =
      (section.type == SectionDirectiveType::BSS) ? 0 : section.size;
  phdr.p_memsz = section.size;
  phdr.p_align = PAGE_SIZE;

  const auto* ptr = reinterpret_cast<const uint8_t*>(&phdr);
  out.insert(out.end(), ptr, ptr + sizeof(phdr));
}

// ===========================================================
//  Section bytes
// ===========================================================

void ElfWriter::emitSection(std::vector<uint8_t>& out, const Section& section) {
  uint64_t currentVaddr = section.virtualAddr;

  for (const auto& stmt : section.items) {
    std::visit(
        [&](const auto& node) {
          using T = std::decay_t<decltype(node)>;

          if constexpr (std::is_same_v<T, Instruction>) {
            std::size_t errorsBefore = encoder.getErrors().size();
            auto bytes = encoder.encode(node, program, currentVaddr);
            if (encoder.getErrors().size() > errorsBefore) {
              for (std::size_t i = errorsBefore; i < encoder.getErrors().size();
                   ++i)
                errors.push_back(encoder.getErrors()[i]);
              return;
            }
            currentVaddr += bytes.size();
            for (uint8_t b : bytes) out.push_back(b);

          } else if constexpr (std::is_same_v<T, DataDirective>) {
            emitDataDirective(out, node);

          } else if constexpr (std::is_same_v<T, Label>) {
          }
        },
        stmt);
  }
}

// ===========================================================
//  Data directive bytes
// ===========================================================

void ElfWriter::emitDataDirective(std::vector<uint8_t>& out,
                                  const DataDirective& dir) {
  // RES* = zeros 
  if (dir.kind == DataDirectiveKind::RESB ||
      dir.kind == DataDirectiveKind::RESW ||
      dir.kind == DataDirectiveKind::RESD ||
      dir.kind == DataDirectiveKind::RESQ) {
    std::size_t unitSize = (dir.kind == DataDirectiveKind::RESB)   ? 1
                           : (dir.kind == DataDirectiveKind::RESW) ? 2
                           : (dir.kind == DataDirectiveKind::RESD) ? 4
                                                                   : 8;
    writePad(out, unitSize * static_cast<std::size_t>(dir.count));
    return;
  }

  // ALIGN — padding zeros
  if (dir.kind == DataDirectiveKind::ALIGN) {
    std::size_t align = static_cast<std::size_t>(dir.count);
    std::size_t pad = (align - (out.size() % align)) % align;
    writePad(out, pad);
    return;
  }

  // DB/DW/DD/DQ
  std::size_t unitSize = (dir.kind == DataDirectiveKind::DB)   ? 1
                         : (dir.kind == DataDirectiveKind::DW) ? 2
                         : (dir.kind == DataDirectiveKind::DD) ? 4
                                                               : 8;

  auto emitOne = [&](const DataValue& val) {
    std::visit(
        [&](const auto& v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, std::string>) {
            for (char c : v) out.push_back(static_cast<uint8_t>(c));
          } else {
            uint64_t raw = static_cast<uint64_t>(v);
            for (std::size_t i = 0; i < unitSize; ++i)
              out.push_back((raw >> (i * 8)) & 0xFF);
          }
        },
        val.value);
  };

  // times = repeat count times
  std::size_t repeat = std::max<std::size_t>(1, dir.count);
  for (std::size_t t = 0; t < repeat; ++t)
    for (const auto& val : dir.values) emitOne(val);
}

// ===========================================================
//  Helpers
// ===========================================================

std::optional<uint64_t> ElfWriter::findEntryPoint() const {
  auto it = program.symbols.find("_start");
  if (it == program.symbols.end())
    return program.sections.empty()
               ? std::nullopt
               : std::optional(program.sections[0].virtualAddr);

  const Symbol& sym = it->second;
  if (sym.sectionIndex >= program.sections.size()) return std::nullopt;

  return program.sections[sym.sectionIndex].virtualAddr + sym.offset;
}

uint32_t ElfWriter::sectionFlags(SectionDirectiveType type) {
  switch (type) {
    case SectionDirectiveType::TEXT:
      return PF_R | PF_X;
    case SectionDirectiveType::DATA:
      return PF_R | PF_W;
    case SectionDirectiveType::BSS:
      return PF_R | PF_W;
  }
  return PF_R;
}

void ElfWriter::write16(std::vector<uint8_t>& out, uint16_t v) {
  out.push_back(v & 0xFF);
  out.push_back(v >> 8);
}

void ElfWriter::write32(std::vector<uint8_t>& out, uint32_t v) {
  for (int i = 0; i < 4; ++i) out.push_back((v >> (i * 8)) & 0xFF);
}

void ElfWriter::write64(std::vector<uint8_t>& out, uint64_t v) {
  for (int i = 0; i < 8; ++i) out.push_back((v >> (i * 8)) & 0xFF);
}

void ElfWriter::writePad(std::vector<uint8_t>& out, std::size_t n) {
  for (std::size_t i = 0; i < n; ++i) out.push_back(0x00);
}

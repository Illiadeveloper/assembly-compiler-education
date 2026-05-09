/// @file Layout.cpp
#include "encoder/Layout.h"

// ELF x86-64 constants
static constexpr uint64_t TEXT_BASE = 0x400000;
static constexpr uint64_t ELF_HEADER_SIZE = 64;  // sizeof(Elf64_Ehdr)
static constexpr uint64_t PHDR_SIZE = 56;        // sizeof(Elf64_Phdr)
static constexpr uint64_t PAGE_SIZE = 0x1000;    // 4KB

Layout::Layout(Program& program) : program(program) {}

const std::vector<CompilerError>& Layout::getErrors() const noexcept {
  return errors;
}

bool Layout::hasErrors() const noexcept { return !errors.empty(); }

void Layout::assign() {
  // Number of PT_LOAD segments equals number of sections
  const std::size_t numSections = program.sections.size();

  // Total file size occupied before the first section
  const uint64_t headersSize = ELF_HEADER_SIZE + PHDR_SIZE * numSections;

  uint64_t fileOffset = alignUp(headersSize, PAGE_SIZE);
  uint64_t vaddr = TEXT_BASE + fileOffset;

  for (auto& section : program.sections) {
    if (section.size == 0) {
      // Skip empty sections
      continue;
    }

    section.fileOffset = fileOffset;
    section.virtualAddr = vaddr;

    // Symbol offsets remain relative to the start of the section.
    // Absolute address can be computed as:
    //
    // symbolAddress = section.virtualAddr + symbol.offset

    fileOffset = alignUp(fileOffset + section.size, PAGE_SIZE);
    vaddr = alignUp(vaddr + section.size, PAGE_SIZE);
  }
}

std::size_t Layout::alignUp(std::size_t value, std::size_t align) {
  if (align == 0) return value;
  return (value + align - 1) & ~(align - 1);
}

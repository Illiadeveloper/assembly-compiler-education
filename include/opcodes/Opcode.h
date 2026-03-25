/// @file Opcode.h
/// @brief Defines the opcode table and mnemonic-to-opcode mapping
/// @details
/// This file contains:
/// 1. "Opcode" enum — all supported instructions
/// 2. "MnemonicToOpcode" — mapping from string mnemonics to "Opcode"
/// 3. "parseMnemonic" function to parse a string into an "Opcode"
#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

/// @brief All supported opcodes
enum class Opcode : uint8_t {
  // === Data movement ===
  MOV,  ///< Move data
  LEA,  ///< Load effective address
  PUSH, ///< Push value to stack
  POP,  ///< Pop value from stack

  // === Arithmetic ===
  ADD,  ///< Addition
  SUB,  ///< Subtraction
  IMUL, ///< Signed multiplication
  MUL,  ///< Unsigned multiplication
  IDIV, ///< Signed division
  DIV,  ///< Unsigned division
  INC,  ///< Increment
  DEC,  ///< Decrement
  NEG,  ///< Negate

  // === Logic / bitwise ===
  AND, ///< Logical AND
  OR,  ///< Logical OR
  XOR, ///< Exclusive OR
  NOT, ///< Logical NOT
  SHL, ///< Shift left
  SHR, ///< Shift right (logical)
  SAR, ///< Shift right (arithmetic)

  // === Compare / test ===
  CMP,  ///< Compare
  TEST, ///< Bitwise test

  // === Control flow ===
  JMP,  ///< Unconditional jump
  JE,   ///< Jump if equal
  JNE,  ///< Jump if not equal
  JL,   ///< Jump if less
  JLE,  ///< Jump if less or equal
  JG,   ///< Jump if greater
  JGE,  ///< Jump if greater or equal
  CALL, ///< Call procedure
  RET,  ///< Return from procedure

  // === Stack / frame ===
  ENTER, ///< Enter stack frame
  LEAVE, ///< Leave stack frame

  // === System / misc ===
  NOP,     ///< No operation
  SYSCALL, ///< System call

  COUNT ///< Total number of opcodes
};

constexpr size_t opcode_count = static_cast<size_t>(Opcode::COUNT);

/// @brief Maps string mnemonics to corresponding Opcode.
/// @details
/// Used for parsing assembler text into "Opcode"
/// @code
/// auto op = parseMnemonic("mov");
/// if(op.has_value()) { /* op.value() == Opcode::MOV */ }
/// @endcode
static const std::unordered_map<std::string, Opcode> MnemonicToOpcode = {
    // Data movement
    {"mov", Opcode::MOV},
    {"lea", Opcode::LEA},
    {"push", Opcode::PUSH},
    {"pop", Opcode::POP},

    // Arithmetic
    {"add", Opcode::ADD},
    {"sub", Opcode::SUB},
    {"imul", Opcode::IMUL},
    {"mul", Opcode::MUL},
    {"idiv", Opcode::IDIV},
    {"div", Opcode::DIV},
    {"inc", Opcode::INC},
    {"dec", Opcode::DEC},
    {"neg", Opcode::NEG},

    // Logic / bitwise
    {"and", Opcode::AND},
    {"or", Opcode::OR},
    {"xor", Opcode::XOR},
    {"not", Opcode::NOT},
    {"shl", Opcode::SHL},
    {"shr", Opcode::SHR},
    {"sar", Opcode::SAR},

    // Compare / test
    {"cmp", Opcode::CMP},
    {"test", Opcode::TEST},

    // Control flow
    {"jump", Opcode::JMP},
    {"jmp", Opcode::JMP}, // jmp == jump
    {"j", Opcode::JMP},   // j == jump
    {"je", Opcode::JE},
    {"jz", Opcode::JE}, // je == jz
    {"jne", Opcode::JNE},
    {"jnz", Opcode::JNE}, // jne == jnz
    {"jl", Opcode::JL},
    {"jle", Opcode::JLE},
    {"jg", Opcode::JG},
    {"jge", Opcode::JGE},

    {"call", Opcode::CALL},
    {"ret", Opcode::RET},

    // Stack/frame
    {"enter", Opcode::ENTER},
    {"leave", Opcode::LEAVE},

    // Misc
    {"nop", Opcode::NOP},
    {"syscall", Opcode::SYSCALL}};

// static std::optional<int> a = 10;

/// @brief Parses a string mnemonic into its corresponding "Opcode"
/// @param s The mnemonic string (e.g., "mov").
/// @return "std::optional<Opcode>": "Opcode" if mnemonic is known, else "std::nullopt".
inline std::optional<Opcode> parseMnemonic(const std::string &s) {
  auto it = MnemonicToOpcode.find(s);
  if (it == MnemonicToOpcode.end())
    return std::nullopt;
  return it->second;
}

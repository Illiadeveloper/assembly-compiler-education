/// @file Registers.h
/// @brief Register definitions and lookup tables
#pragma once

#include "common/OperandTypes.h"
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

/// @brief Enumeration of supported CPU registers
enum class Register {
  // 64-bits
  RAX,
  RBX,
  RCX,
  RDX,
  RSI,
  RDI,
  RBP,
  RSP,

  // 32-bits
  EAX,
  EBX,
  ECX,
  EDX,

  // 16-bits
  AX,
  BX,
  CX,
  DX,

  // 8-bits
  AL,
  BL,
  CL,
  DL
};

/// @brief Metadata describing a register
struct RegisterInfo {
  Register reg;     ///< Register identifier (RAX, AL, etc.)
  OperandSize size; ///< Size of the register in bits
  uint8_t code;     ///< ModRM register code (0..7)
};

/// @brief Maps textual register names to Register enum values
inline const std::unordered_map<std::string, Register> RegisterTable = {
    {"rax", Register::RAX}, {"rbx", Register::RBX}, {"rcx", Register::RCX},
    {"rdx", Register::RDX}, {"rsi", Register::RSI}, {"rdi", Register::RDI},
    {"rbp", Register::RBP}, {"rsp", Register::RSP}, {"eax", Register::EAX},
    {"ebx", Register::EBX}, {"ecx", Register::ECX}, {"edx", Register::EDX},
    {"ax", Register::AX},   {"bx", Register::BX},   {"cx", Register::CX},
    {"dx", Register::DX},   {"al", Register::AL},   {"bl", Register::BL},
    {"cl", Register::CL},   {"dl", Register::DL},
};

/// @brief Table containing metadata for every register
constexpr std::array<RegisterInfo, 20> RegisterInfoTable = {{
    // 64 - bit
    {Register::RAX, OperandSize::B64, 0},
    {Register::RBX, OperandSize::B64, 3},
    {Register::RCX, OperandSize::B64, 1},
    {Register::RDX, OperandSize::B64, 2},
    {Register::RSI, OperandSize::B64, 6},
    {Register::RDI, OperandSize::B64, 7},
    {Register::RBP, OperandSize::B64, 5},
    {Register::RSP, OperandSize::B64, 4},

    // 32-bit
    {Register::EAX, OperandSize::B32, 0},
    {Register::EBX, OperandSize::B32, 3},
    {Register::ECX, OperandSize::B32, 1},
    {Register::EDX, OperandSize::B32, 2},

    // 16-bit
    {Register::AX, OperandSize::B16, 0},
    {Register::BX, OperandSize::B16, 3},
    {Register::CX, OperandSize::B16, 1},
    {Register::DX, OperandSize::B16, 2},

    // 8-bit
    {Register::AL, OperandSize::B8, 0},
    {Register::BL, OperandSize::B8, 3},
    {Register::CL, OperandSize::B8, 1},
    {Register::DL, OperandSize::B8, 2},
}};

/// @brief Returns metadata for the given register
inline constexpr const RegisterInfo &getRegisterInfo(Register r) {
  return RegisterInfoTable[static_cast<size_t>(r)];
}

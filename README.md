# 🛠️ Assembly Compiler (Educational)
> A from-scratch x86-64 assembly compiler — built for learning how compilers actually work under the hood.
> Transforms NASM-style assembly source code into a Linux ELF64 executable binary.

## Quick Start
```bash
cmake -S . -B build 
cmake --build build
./build/src/AssemblyCompiler
```

---

## Docs / Tests

**Docs**
```bash
doxygen docs/Doxyfile.dev
# open html/index.html in browser
```

**Tests**
```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build/tests --output-on-failure
```

---

## What is this?

This project is an educational compiler that takes NASM-style x86-64 assembly source code
and compiles it into a runnable Linux ELF64 executable — completely from scratch in C++

The goal is to understand each stage deeply by implementing it without any compiler frameworks or libraries

---
## Compilation Pipeline

```
Source Code (.asm)
       │
       ▼
   [ Lexer ]          → Tokenizes raw text into a stream of tokens
       │
       ▼
   [ Parser ]         → Builds a flat AST (FlatProgram)
       │
       ▼
   [ Grouper ]        → Groups statements into sections (.text, .data, .bss)
                        Builds SymbolTable (labels, equ constants)
       │
       ▼
   [ SizeResolver ]   → Computes exact instruction sizes and symbol offsets
                        Runs iterative passes until offsets stabilize
       │
       ▼
   [ Layout ]         → Assigns virtual addresses and file offsets to sections
                        Page-aligns sections (4KB boundaries)
       │
       ▼
   [ ElfWriter ]      → Emits ELF64 header, program headers, and section bytes
                        Uses InstructionEncoder to produce x86-64 machine code
       │
       ▼
  ELF64 Executable
```

---
## Architecture

```
src/
├── lexer/            
│   └── Lexer         — Lexical analysis (Lexer)
├── parser/           
│   └── Parser        — Syntax analysis (Parser, MemoryExpression)
├── grouper/          
│   └── Grouper       — IR grouping (Grouper)
├── encoder/          — Size resolution, layout, ELF output
│   ├── InstructionEncoder  — x86-64 instruction → bytes
│   ├── SizeResolver        — iterative offset computation
│   ├── Layout              — virtual address assignment
│   └── ElfWriter           — ELF64 binary generation
├── opcodes/          — Declarative opcode encoding table
│   ├── Opcode              — opcode enum + mnemonic map
│   ├── OpcodePattern       — encoding pattern structure
│   ├── OpcodeFactories     — pattern builder structs
│   └── OpcodeTable         — global pattern table
├── ast/              — AST node types
│   ├── Instruction         — instruction + operands
│   ├── Directive           — data/section/equ/global directives
│   └── Statement           — top-level statement variant
├── ir/               — Intermediate representation
│   └── Program             — sections + symbol table
└── common/           — Shared types
    ├── Token               — token types and source spans
    ├── Registers           — register enum and info table
    ├── OperandTypes        — operand size/kind enums
    ├── Helper              — helping functions
    └── CompilerError       — unified error reporting
```

---

## Supported Instructions

| Category | Instructions |
|----------|-------------|
| Data movement | `MOV`, `LEA`, `PUSH`, `POP` |
| Arithmetic | `ADD`, `SUB`, `IMUL`, `MUL`, `IDIV`, `DIV`, `INC`, `DEC`, `NEG` |
| Logic / Bitwise | `AND`, `OR`, `XOR`, `NOT`, `SHL`, `SHR`, `SAR` |
| Compare / Test | `CMP`, `TEST` |
| Control flow | `JMP`, `JE`, `JNE`, `JL`, `JLE`, `JG`, `JGE`, `CALL`, `RET` |
| Stack / Frame | `ENTER`, `LEAVE` |
| System | `NOP`, `SYSCALL` |

## Supported Directives

| Directive | Description |
|-----------|-------------|
| `section .text / .data / .bss` | Section declaration |
| `global name` | Export symbol |
| `name equ value` | Constant definition |
| `db / dw / dd / dq` | Define initialized data |
| `resb / resw / resd / resq` | Reserve uninitialized data |
| `align N` | Align to N-byte boundary |
| `times N` | Repeat next directive N times |

---

## Example

See [`example.asm`](example.asm) for a full working example.

A minimal program that writes to stdout and exits:

```asm
section .text
global _start
_start:
    mov rax, 60   ; sys_exit
    xor rdi, rdi  ; exit code 0
    syscall
```

To compile and run:
```bash
./build/src/AssemblyCompiler example.asm output
./output
echo $?   # 0
```

---

## Progress

| Stage          | Status      |
|----------------|-------------|
| Lexer          | 🟢 Done     |
| Parser         | 🟢 Done     |
| Grouper        | 🟢 Done     |
| SizeResolver   | 🟢 Done     |
| Layout         | 🟢 Done     |
| InstructionEncoder | 🟢 Done |
| ElfWriter      | 🟢 Done     |

---

## Learning Goals

- Understand how x86-64 instructions are encoded into raw bytes (REX prefix, ModRM, SIB, displacement, immediates)
- Learn the structure of Linux ELF64 executables (ELF header, program headers, sections)
- Practice building a multi-stage compiler pipeline with clean separation of concerns
- Write self-documenting C++ and generate API docs with Doxygen

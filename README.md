# 🛠️ Assembly Compiler (Educational)

> A from-scratch compiler for a subset of x86-like Assembly — built for learning how compilers actually work under the hood

## Quick Start

```bash
cmake -S . -B build 
cmake --build build 
./build/src/AssemblyCompiler
```

---

## Docs/Tests

Docs
```bash
cmake -S . -B build -DBUILD_DOCS=ON
cmake --build build
[browser] docs/html/index.html
```

Tests
```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## What is this?

This project is an educational compiler that takes Assembly-like source code and processes it through the classic compiler pipeline: **Lexer → Parser → Code Generation**. The goal is to understand each stage deeply by implementing it from scratch in C++.

---

## Architecture

```
Source Code (.asm)
      │
      ▼
  [ Lexer ]         → Tokenizes raw text into a stream of tokens
      │
      ▼
  [ Parser ]        → Builds an Abstract Syntax Tree (AST)
      │
      ▼
  [ Code Gen ]      → Emits output (object code / IR)
```

---

## Progress

| Stage           | Status        | Branch       |
|-----------------|---------------|--------------|
| Lexer           | 🟢 Done | `lexer`      |
| Parser          | 🟢 Done | `parser`     |
| Code Generation | 🟡 Not started | `codegen`    |

---

## Branch Structure

Each major compiler stage lives in its own branch with its own focused `README` section:

- **`feature/lexer`** — Tokenization logic and token definitions
- **`feature/parser`** — AST construction, grammar rules
- **`feature/codegen`** — Output generation

---

## Learning Goals
- Understand the structure of ELF files and how sections (`.text`, `.data`, `.symtab`) are laid out
- Learn how Assembly instructions are encoded into raw bytes (opcodes, ModR/M, SIB, displacement)
- Explore how a recursive descent parser turns a flat token stream into a structured AST
- Practice writing self-documenting C++ and generating API docs with Doxygen

---

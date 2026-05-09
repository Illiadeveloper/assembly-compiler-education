/// @file Compiler.h
/// @brief Defines the top-level assembler compiler pipeline
#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "common/CompilerError.h"

/// @brief Configuration options for the compiler pipeline
struct CompilerOptions {
  /// Path to input assembly source file
  std::string inputPath;

  /// Path to output executable file
  std::string outputPath;
};

/// @brief High-level orchestrator of the assembler pipeline
///
/// The Compiler coordinates all compilation stages:
/// - Source loading
/// - Lexing/parsing
/// - IR construction
/// - Size resolution
/// - Layout computation
/// - ELF generation
///
/// It also handles:
/// - Error reporting
/// - Pipeline execution flow
/// - Input/output file management
class Compiler {
 public:
  /// @brief Creates a new compiler instance
  ///
  /// @param options Compiler configuration options
  Compiler(CompilerOptions options);

  /// @brief Executes the full compilation pipeline
  void run();

 private:
  /// @brief Prints a list of compilation errors
  ///
  /// @param errors Errors to print
  /// @param showSpan Whether source location should be shown
  /// @param starter Optional prefix printed before errors
  void printAllErrors(const std::vector<CompilerError>& errors,
                      bool showSpan = true, const std::string& starter = "");

  /// @brief Reads entire file into memory
  ///
  /// @param path File path
  ///
  /// @return File contents as string
  std::string readFile(const std::filesystem::path& path);

  /// Compiler configuration
  CompilerOptions options;
};

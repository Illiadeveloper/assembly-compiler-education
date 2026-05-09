/// @file Grouper.cpp
#include "Compiler.h"

#include <unistd.h>

#include <iostream>
#include <stdexcept>

#include "ast/Statement.h"
#include "encoder/ElfWriter.h"
#include "encoder/InstructionEncoder.h"
#include "encoder/Layout.h"
#include "encoder/SizeResolver.h"
#include "grouper/Grouper.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"

Compiler::Compiler(CompilerOptions options) : options(options) {}

void Compiler::printAllErrors(const std::vector<CompilerError>& errors,
                              bool showSpan, const std::string& starter) {
  for (const auto& error : errors) {
    std::cerr << starter;

    if (showSpan) {
      std::cerr << "(line: " << error.span.line
                << "; column: " << error.span.column
                << "; length: " << error.span.length << ")";
    }

    std::cerr << ": " << error.message << '\n';
  }
}

std::string Compiler::readFile(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);

  if (!file) {
    throw std::runtime_error("Failed to open file: " + path.string());
  }

  file.seekg(0, std::ios::end);
  size_t size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::string result(size, '\0');
  file.read(result.data(), size);

  return result;
}

void Compiler::run() {
  std::string content = readFile(options.inputPath);

  Lexer lexer(content);
  if (lexer.hasErrors()) {
    printAllErrors(lexer.getErrors(), true, "[LEXER]");
    throw std::runtime_error("Compilation failed at lexer stage");
  }

  Parser parser(lexer.tokenize());
  FlatProgram flatProgram = parser.parse();
  if (parser.hasErrors()) {
    printAllErrors(parser.getErrors(), true, "[PARSER]");
    throw std::runtime_error("Compilation failed at parser stage");
  }

  Grouper grouper(flatProgram);
  Program ir = grouper.group();
  if (grouper.hasErrors()) {
    printAllErrors(grouper.getErrors(), true, "[GROUPER]");
    throw std::runtime_error("Compilation failed at grouper stage");
  }

  InstructionEncoder encoder;
  SizeResolver resolver(ir, encoder);
  resolver.resolve();

  if (encoder.hasErrors()) {
    printAllErrors(encoder.getErrors(), true, "[ENCODER]");
    throw std::runtime_error("Compilation failed at encoder stage");
  }

  if (resolver.hasErrors()) {
    printAllErrors(resolver.getErrors(), false, "[SIZE_RESOLVER]");
    throw std::runtime_error("Compilation failed at size resolver stage");
  }

  Layout layout(ir);
  layout.assign();

  if (layout.hasErrors()) {
    printAllErrors(layout.getErrors(), false, "[LAYOUT]");
    throw std::runtime_error("Compilation failed at layot stage");
  }

  ElfWriter writer(ir, encoder);
  writer.write(options.outputPath);

  if (encoder.hasErrors()) {
    printAllErrors(encoder.getErrors(), true, "[ENCODER]");
    throw std::runtime_error("Compilation failed at encoder stage");
  }

  if (writer.hasErrors()) {
    printAllErrors(writer.getErrors(), false, "[WRITER]");
    throw std::runtime_error("Compilation failed at writer stage");
  }
}

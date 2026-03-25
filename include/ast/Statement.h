/// @file Statement.h
/// @brief Top-level AST statements
#pragma once

#include "Directive.h"
#include "Instruction.h"
#include <variant>
#include <vector>

/// Label definition (e.g. 'main:', 'loop:') 
struct Label {
  std::string name; ///< Label name without colon
  SourceSpan span;
};

/// Any top-level assembly statement
using Statement = std::variant<Instruction,      //
                               Label,            //
                               SectionDirective, //
                               EquDirective,     //
                               GlobalDirective,  //
                               DataDirective>;

/// Flat list of statements representing the parsed program, in source orde
using FlatProgram = std::vector<Statement>;

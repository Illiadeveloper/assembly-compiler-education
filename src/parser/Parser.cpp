/// @file Parser.cpp
#include "parser/Parser.h"
#include "ast/Directive.h"
#include "ast/Instruction.h"
#include "ast/Statement.h"
#include "common/CompilerError.h"
#include "common/Helper.h"
#include "common/OperandTypes.h"
#include "common/Registers.h"
#include "common/Token.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "common/Helper.h"
#include "opcodes/Opcode.h"

void Parser::addError(const SourceSpan &span, const std::string &msg) {
  errors.push_back({ErrorStage::PARSER, span, msg});
}

const Token &Parser::peek(size_t offset) const {
  if (offset + currentToken >= tokens.size()) {
    return tokens[tokens.size() - 1];
  }
  return tokens[offset + currentToken];
}

const Token &Parser::advance() {
  if (currentToken >= tokens.size()) {
    return tokens[tokens.size() - 1];
  }
  return tokens[currentToken++];
}

bool Parser::endOfFile() const {
  return currentToken >= tokens.size() ||
         tokens[currentToken].type == TokenType::END_OF_FILE;
}

bool Parser::match(TokenType type, size_t offset) const {
  return peek(offset).type == type;
}
void Parser::skipToNextLine() {
  while (!endOfFile() && peek().type != TokenType::NEWLINE) {
    advance();
  }
  if (match(TokenType::NEWLINE)) {
    advance();
  }
}

// =============== Is Directives ==================
bool Parser::tokenIsDataDirective(const Token &token) const {
  if (token.type != TokenType::IDENTIFIER)
    return false;
  auto key = Helper::stringToLower(std::get<std::string>(token.value));
  auto it = DataDirectiveTable.find(key);
  return it != DataDirectiveTable.end();
}

bool Parser::isGlobalDirective() const {
  if (peek().type != TokenType::IDENTIFIER)
    return false;
  auto key = Helper::stringToLower(std::get<std::string>(peek().value));
  auto it = DirectiveKeywordTable.find(key);
  return it != DirectiveKeywordTable.end() &&
         it->second == DirectiveKeyword::GLOBAL;
}

std::optional<int64_t> Parser::parseSignedInteger() {
  if (match(TokenType::MINUS) && match(TokenType::NUMBER, 1)) {
    advance(); // consume '-'
    Token numberToken = advance();
    return -std::get<int64_t>(numberToken.value);
  }
  if (match(TokenType::NUMBER)) {
    Token numberToken = advance();
    return std::get<int64_t>(numberToken.value);
  }
  return std::nullopt;
}

bool Parser::isDataDirective() const {
  // db ...
  if (tokenIsDataDirective(peek()))
    return true;

  // label db ....
  if (peek().type == TokenType::IDENTIFIER && tokenIsDataDirective(peek(1)))
    return true;

  // times 16 db 42
  if (peek().type == TokenType::IDENTIFIER) {
    auto key = Helper::stringToLower(std::get<std::string>(peek().value));
    auto it = DirectiveKeywordTable.find(key);
    if (it != DirectiveKeywordTable.end() &&
        it->second == DirectiveKeyword::TIMES)
      return true;
  }

  // msg times 16 db 42
  if (peek().type == TokenType::IDENTIFIER &&
      peek(1).type == TokenType::IDENTIFIER) {
    auto key = Helper::stringToLower(std::get<std::string>(peek(1).value));
    auto it = DirectiveKeywordTable.find(key);
    if (it != DirectiveKeywordTable.end() &&
        it->second == DirectiveKeyword::TIMES)
      return true;
  }

  return false;
}

bool Parser::isSectionDirective() const {
  if (peek().type != TokenType::IDENTIFIER)
    return false;
  auto key = Helper::stringToLower(std::get<std::string>(peek().value));
  auto it = DirectiveKeywordTable.find(key);
  return it != DirectiveKeywordTable.end() &&
         it->second == DirectiveKeyword::SECTION;
}

bool Parser::isEquDirection() const {
  if (peek().type != TokenType::IDENTIFIER ||
      peek(1).type != TokenType::IDENTIFIER)
    return false;

  auto key = Helper::stringToLower(std::get<std::string>(peek(1).value));
  auto it = DirectiveKeywordTable.find(key);
  return it != DirectiveKeywordTable.end() &&
         it->second == DirectiveKeyword::EQU;
}

// ==========================================================

bool Parser::isLabelDeclaration() const {
  const Token &token = peek();

  if (token.type != TokenType::IDENTIFIER)
    return false;

  if (tokenIsRegister(token))
    return false;

  return match(TokenType::COLON, 1);
}

bool Parser::isInstruction() const {
  Token opcodeToken = peek();
  if (opcodeToken.type != TokenType::IDENTIFIER)
    return false;

  auto key = Helper::stringToLower(std::get<std::string>(opcodeToken.value));
  return parseMnemonic(key).has_value();
}

bool Parser::tokenIsRegister(const Token &token) const {
  if (token.type != TokenType::IDENTIFIER)
    return false;
  std::string registerString = std::get<std::string>(token.value);
  registerString = Helper::stringToLower(registerString);
  return RegisterTable.count(registerString) > 0;
}

bool Parser::isRegister() const { return tokenIsRegister(peek()); }

bool Parser::isMemoryOperand() const {
  if (match(TokenType::LBRACKET))
    return true;

  // identifier + "["   (example "dword [")
  if (match(TokenType::IDENTIFIER) && match(TokenType::LBRACKET, 1)) {
    const std::string &id = std::get<std::string>(peek(0).value);
    if (SizePrefixMap.count(Helper::stringToLower(id)))
      return true;
    return false;
  }

  // identifier + identifier + "["   (example "dword ptr [")
  if (match(TokenType::IDENTIFIER) && match(TokenType::IDENTIFIER, 1) &&
      match(TokenType::LBRACKET, 2)) {
    const std::string &id1 = std::get<std::string>(peek(0).value);
    const std::string &id2 = std::get<std::string>(peek(1).value);
    std::string l1 = Helper::stringToLower(id1);
    std::string l2 = Helper::stringToLower(id2);
    if (SizePrefixMap.count(l1) && (l2 == "ptr"))
      return true;
    return false;
  }

  return false;
}

bool Parser::isImmediate() const {
  if (match(TokenType::NUMBER))
    return true;
  if (match(TokenType::MINUS) && match(TokenType::NUMBER, 1))
    return true;
  return false;
}

bool Parser::isLabelOperand() const {
  // At this point registers, memory operands and immediates
  // have already been checked, so any IDENTIFIER is treated
  // as a symbol reference
  const Token &token = peek();

  if (token.type != TokenType::IDENTIFIER)
    return false;

  return true;
}

std::unique_ptr<MemoryExpression::Node> Parser::parseExpression(int rbp) {
  Token token = advance();
  std::unique_ptr<MemoryExpression::Node> left;

  switch (token.type) {
  case TokenType::NUMBER: {
    auto node = std::make_unique<MemoryExpression::Node>();
    node->kind = MemoryExpression::Node::NUMBER;
    node->value = std::get<int64_t>(token.value);
    left = std::move(node);
    break;
  }
  case TokenType::IDENTIFIER: {
    auto node = std::make_unique<MemoryExpression::Node>();
    std::string s = std::get<std::string>(token.value);

    std::string lower = Helper::stringToLower(s);
    if (tokenIsRegister(token)) {
      node->kind = MemoryExpression::Node::REGISTER;
      node->name = lower;
    } else {
      node->kind = MemoryExpression::Node::IDENTIFIER;
      node->name = lower;
    }

    left = std::move(node);
    break;
  }
  case TokenType::MINUS: {
    auto node = std::make_unique<MemoryExpression::Node>();
    node->kind = MemoryExpression::Node::UNARY;
    node->operand = '-';
    node->right = parseExpression(/* Need to change magic number*/ 50);
    left = std::move(node);
    break;
  }
  case TokenType::LPAREN: {
    left = parseExpression();
    if (!match(TokenType::RPAREN))
      addError(token.span, "expected ')'");
    else
      advance(); // consume ')'
    break;
  }
  default:
    addError(token.span, "unexpected token in expression");
    // create dummy node to continue
    left = std::make_unique<MemoryExpression::Node>();
    left->kind = MemoryExpression::Node::NUMBER;
    left->value = 0;
    break;
  }

  while (rbp < MemoryExpression::bindingPower(peek().type)) {
    Token operand = advance();
    int bp = MemoryExpression::bindingPower(operand.type);

    auto node = std::make_unique<MemoryExpression::Node>();
    node->kind = MemoryExpression::Node::OPERAND;
    node->operand = std::get<std::string>(operand.value)[0];
    node->left = std::move(left);
    node->right = parseExpression(bp);
    left = std::move(node);
  }

  return left;
}

void Parser::flattenAST(
    MemoryExpression::Node *node, int sign,
    std::vector<std::pair<int, MemoryExpression::Node *>> &out) {
  if (!node)
    return;
  if (node->kind == MemoryExpression::Node::OPERAND && node->operand == '+') {
    flattenAST(node->left.get(), sign, out);
    flattenAST(node->right.get(), sign, out);
  } else if (node->kind == MemoryExpression::Node::OPERAND &&
             node->operand == '-') {
    flattenAST(node->left.get(), sign, out);
    flattenAST(node->right.get(), -sign, out);
  } else if (node->kind == MemoryExpression::Node::UNARY &&
             node->operand == '-') {
    flattenAST(node->right.get(), -sign, out);
  } else {
    out.emplace_back(sign, node);
  }
}

std::optional<MemoryOperand> Parser::parseMemoryOperand() {
  // std::cout << "PARSE MEMORY OPERAND..." << std::endl;
  Token token = advance();
  OperandSize size = OperandSize::ANY;

  if (token.type == TokenType::IDENTIFIER) {
    auto key = Helper::stringToLower(std::get<std::string>(token.value));
    if (key != PTR_KEYWORD) {
      auto it = SizePrefixMap.find(key);
      if (it == SizePrefixMap.end()) {
        addError(token.span, "Unknown prefix size of memory operand");
        return std::nullopt;
      }
      size = it->second;
    }
    token = advance();
  }

  if (token.type == TokenType::IDENTIFIER) {
    auto key = Helper::stringToLower(std::get<std::string>(token.value));
    if (key != PTR_KEYWORD) {
      addError(token.span,
               "Unknown prefix size of memory operand(Excpecting \"ptr\")");
      return std::nullopt;
    }
    token = advance();
  }

  if (token.type != TokenType::LBRACKET) {
    addError(token.span,
             "Unknown prefix size of memory operand(Excpecting \"[\")");
    return std::nullopt;
  }

  auto root = parseExpression();

  std::vector<std::pair<int, MemoryExpression::Node *>> terms;
  flattenAST(root.get(), +1, terms);

  MemoryOperand out;
  bool haveBase = false;
  bool haveIndex = false;
  int64_t disp = 0;
  std::vector<SymbolTerm> dispExprs;

  for (auto &p : terms) {
    int sign = p.first;
    MemoryExpression::Node *term = p.second;

    if (term->kind == MemoryExpression::Node::NUMBER) {
      disp += sign * term->value;
      continue;
    }

    if (term->kind == MemoryExpression::Node::IDENTIFIER) {
      dispExprs.push_back({sign, term->name});
      continue;
    }

    if (term->kind == MemoryExpression::Node::REGISTER) {
      if (sign < 0) {
        addError(peek().span,
                 "negated register term (unsupported): " + term->name);
        return std::nullopt;
      }
      if (!haveBase) {
        std::string registerString = Helper::stringToLower(term->name);
        auto it = RegisterTable.find(registerString);
        if (it == RegisterTable.end()) {
          addError(peek().span, "unknown register: " + registerString);
          return std::nullopt;
        }
        out.base = it->second;
        haveBase = true;
      } else if (!haveIndex) {

        std::string registerString = Helper::stringToLower(term->name);
        auto it = RegisterTable.find(registerString);
        if (it == RegisterTable.end()) {
          addError(peek().span, "unknown register: " + registerString);
          return std::nullopt;
        }
        out.index = it->second;
        out.scale = 1;
        haveIndex = true;
      } else {
        addError(peek().span, "too many registers in memory operand");
        return std::nullopt;
      }
      continue;
    }

    if (term->kind == MemoryExpression::Node::OPERAND && term->operand == '*') {
      MemoryExpression::Node *left = term->left.get();
      MemoryExpression::Node *right = term->right.get();

      if (!left || !right) {
        addError(peek().span, "malformed multiplication term");
        return std::nullopt;
      }

      MemoryExpression::Node *regNode = nullptr;
      MemoryExpression::Node *numNode = nullptr;

      if (left->kind == MemoryExpression::Node::REGISTER &&
          right->kind == MemoryExpression::Node::NUMBER) {
        regNode = left;
        numNode = right;
      } else if (left->kind == MemoryExpression::Node::NUMBER &&
                 right->kind == MemoryExpression::Node::REGISTER) {
        regNode = right;
        numNode = left;
      } else {
        addError(peek().span,
                 "unsupported multiplication term in memory operand");
        return std::nullopt;
      }

      if (sign < 0) {
        addError(peek().span,
                 "negated index term (negative scale) is not allowed");
        return std::nullopt;
      }

      int scale = static_cast<int>(numNode->value);
      if (scale != 1 && scale != 2 && scale != 4 && scale != 8) {
        addError(peek().span, "invalid scale: " + std::to_string(scale));
        return std::nullopt;
      }

      if (!haveIndex) {
        // std::cout << term->kind << std::endl;
        std::string registerString = Helper::stringToLower(regNode->name);
        auto it = RegisterTable.find(registerString);
        if (it == RegisterTable.end()) {
          addError(peek().span, "unknown register: " + registerString);
          return std::nullopt;
        }
        out.index = it->second;
        out.scale = scale;
        haveIndex = true;
      } else {
        addError(peek().span, "multiple index terms found");
        return std::nullopt;
      }
      continue;
    }

    addError(peek().span, "unsupported term in memory operand");
    return std::nullopt;
  }

  out.displacement = disp;
  out.symbols = std::move(dispExprs);
  out.size = size;
  // std::cout << (int)peek().type << std::endl;
  if (!match(TokenType::RBRACKET)) {
    addError(peek().span, "expected ']'");
    return std::nullopt;
  }

  advance(); // consume ']'
  return out;
}

Register Parser::parseRegister() {
  Token token = advance();
  auto key = Helper::stringToLower(std::get<std::string>(token.value));
  Register reg = RegisterTable.find(key)->second;
  return reg;
}

Label Parser::parseLabelOperand() {
  Token token = advance();
  auto name = Helper::stringToLower(std::get<std::string>(token.value));
  Label label{.name = name, .span = token.span};
  return label;
}

Label Parser::parseLabelDeclaration() {
  Token token = advance();
  auto name = Helper::stringToLower(std::get<std::string>(token.value));
  Label label{.name = name, .span = token.span};
  advance(); // consume ':'
  return label;
}

std::optional<GlobalDirective> Parser::parseGlobalDirective() {
  advance();
  Token token = advance();
  if (token.type != TokenType::IDENTIFIER) {
    addError(token.span, "Excpecting correct label name");
    return std::nullopt;
  }
  auto name = Helper::stringToLower(std::get<std::string>(token.value));
  return GlobalDirective{.name = name, .span = token.span};
}

std::optional<DataDirective> Parser::parseDataDirective() {
  DataDirective out;
  out.count = 1;
  Token startToken = peek();

  // --- Possible prefixes:
  // 1) db ... // I don't support this version
  // 2) label db ...
  // 3) times N db ...
  // 4) label times N db ...

  // Case: "label db ..." or "label times N db ..."
  if (match(TokenType::IDENTIFIER) && match(TokenType::IDENTIFIER, 1) &&
      (tokenIsDataDirective(peek(1)) ||
       (peek(1).type == TokenType::IDENTIFIER &&
        DirectiveKeywordTable.count(
            Helper::stringToLower(std::get<std::string>(peek(1).value))) &&
        DirectiveKeywordTable.at(Helper::stringToLower(std::get<std::string>(
            peek(1).value))) == DirectiveKeyword::TIMES))) {
    // label in the start
    Token labelTok = advance();
    out.label = Helper::stringToLower(std::get<std::string>(labelTok.value));
    startToken = labelTok;
    // now we have either "db ..." or "times N db ..."
  }

  // Case "times N db ..."
  if (match(TokenType::IDENTIFIER)) {
    auto key = Helper::stringToLower(std::get<std::string>(peek().value));
    auto it = DirectiveKeywordTable.find(key);
    if (it != DirectiveKeywordTable.end() &&
        it->second == DirectiveKeyword::TIMES) {
      advance(); // consume 'times'
      if (!match(TokenType::NUMBER)) {
        addError(peek().span, "expected number after 'times'");
        return std::nullopt;
      }
      Token numberToken = advance();
      int64_t timesCount = std::get<int64_t>(numberToken.value);
      if (timesCount <= 0) {
        addError(numberToken.span, "'times' count must be positive");
        return std::nullopt;
      }
      out.count = timesCount;
      // Now expecting only db ...
    }
  }

  // Now expecting Data Directives (db/dw/dd/dq/res*/align)
  if (!match(TokenType::IDENTIFIER)) {
    addError(peek().span, "expected data directive (db/dw/dd/dq/res*/align)");
    return std::nullopt;
  }

  Token directiveToken = advance();
  std::string dirKey =
      Helper::stringToLower(std::get<std::string>(directiveToken.value));
  auto dit = DataDirectiveTable.find(dirKey);
  if (dit == DataDirectiveTable.end()) {
    addError(directiveToken.span, "unknown data directive: " + dirKey);
    return std::nullopt;
  }
  out.kind = dit->second;
  out.span = startToken.span;

  // --- Unique RES* and ALIGN ---
  if (out.kind == DataDirectiveKind::RESB ||
      out.kind == DataDirectiveKind::RESW ||
      out.kind == DataDirectiveKind::RESD ||
      out.kind == DataDirectiveKind::RESQ) {
    // syntax: resb N
    if (!match(TokenType::NUMBER)) {
      addError(peek().span, "expected size/count after '" + dirKey + "'");
      return std::nullopt;
    }
    Token numberToken = advance();
    int64_t number = std::get<int64_t>(numberToken.value);
    if (number < 0) {
      addError(numberToken.span, "res* count must be non-negative");
      return std::nullopt;
    }
    out.count = number;
    return out;
  }

  if (out.kind == DataDirectiveKind::ALIGN) {
    // syntax: align N
    if (!match(TokenType::NUMBER)) {
      addError(peek().span, "expected alignment value after 'align'");
      return std::nullopt;
    }
    Token numberToken = advance();
    int64_t number = std::get<int64_t>(numberToken.value);
    if (number <= 0) {
      addError(numberToken.span, "align value must be positive");
      return std::nullopt;
    }
    out.count = number;
    return out;
  }

  while (!endOfFile()) {
    if (match(TokenType::STRING)) {
      Token stringToken = advance();
      DataValue dataValue;
      dataValue.value = std::get<std::string>(stringToken.value);
      out.values.push_back(std::move(dataValue));
    } else {
      // number, maybe with unary monus
      auto maybeNum = parseSignedInteger();
      if (maybeNum.has_value()) {
        DataValue dataValue;
        dataValue.value = maybeNum.value();
        out.values.push_back(std::move(dataValue));
      } else if (match(TokenType::IDENTIFIER)) {
        // symbol, label ..
        Token identifierToken = advance();
        DataValue dataValue;
        dataValue.value =
            Helper::stringToLower(std::get<std::string>(identifierToken.value));
        out.values.push_back(std::move(dataValue));
      } else {
        // nothing interesting
        break;
      }
    }

    // if comma continue
    if (match(TokenType::COMMA)) {
      advance(); // consume comma
      continue;
    } else {
      break;
    }
  }

  if (out.values.empty()) {
    addError(peek().span, "expected value(s) after '" + dirKey + "'");
    return std::nullopt;
  }

  return out;
}

std::optional<SectionDirective> Parser::parseSectionDirective() {
  Token sectionToken = advance();
  if (!match(TokenType::IDENTIFIER) && !match(TokenType::DOT)) {
    addError(peek().span, "expected section name after 'section'");
    return std::nullopt;
  }

  Token nameToken = advance();
  std::string name;
  if (nameToken.type == TokenType::DOT) {
    if (!match(TokenType::IDENTIFIER)) {
      addError(peek().span,
               "expected identifier after '.' in section directive");
      return std::nullopt;
    }
    Token idToken = advance();
    name = std::string(".") +
           Helper::stringToLower(std::get<std::string>(idToken.value));
  } else {
    name = Helper::stringToLower(std::get<std::string>(nameToken.value));
  }

  auto it = SectionTable.find(name);
  if (it == SectionTable.end()) {
    addError(peek().span, "Unknown section: " + name);
    return std::nullopt;
  }

  SectionDirective result;
  result.span = sectionToken.span;
  result.type = it->second;
  return result;
}

std::optional<EquDirective> Parser::parseEquDirective() {
  Token nameToken = advance(); // name
  Token equToken = advance();  // skipping equ
  // Token valueToken = advance();

  auto value = parseSignedInteger();
  if (!value) {
    addError(peek().span, "expected integer after 'equ'");
    return std::nullopt;
  }

  EquDirective result;
  // result.name = Helper::stringToLower(std::get<std::string>(nameToken.value));
  result.name = std::get<std::string>(nameToken.value);
  result.value = *value;
  result.span = nameToken.span;

  return result;
}

std::optional<Operand> Parser::parseOperand() {
  if (isRegister()) {
    // Operand operand;
    SourceSpan currentSpan = peek().span;
    Register reg = parseRegister();
    RegisterInfo info = getRegisterInfo(reg);
    // operand.makeReg(info.reg, info.size, currentSpan);
    return Operand::makeReg(info.reg, info.size, currentSpan);
  }
  if (isMemoryOperand()) {
    // Operand operand;
    SourceSpan currentSpan = peek().span;
    std::optional<MemoryOperand> mem = parseMemoryOperand();
    if (!mem) {
      return std::nullopt; // Error has been already added
    }
    // operand.makeMem(*mem, mem->size, currentSpan);
    return Operand::makeMem(*mem, mem->size, currentSpan);
    ;
  }
  if (isLabelOperand()) {
    // Operand operand;
    Label label = parseLabelOperand();
    // operand.makeLabel(label.name, label.span);
    return Operand::makeLabel(label.name, label.span);
  }
  if (isImmediate()) {
    SourceSpan currentSpan = peek().span;
    auto maybeValue = parseSignedInteger();
    if (!maybeValue) {
      addError(peek().span, "expected integer immediate");
      return std::nullopt;
    }
    // Operand operand;
    return Operand::makeImm(*maybeValue, OperandSize::ANY, currentSpan);
  }
  addError(peek().span,
           "expected operand (register, memory, immediate or label)");
  return std::nullopt;
}

std::optional<Instruction> Parser::parseInstruction() {
  Token opcodeToken = advance();
  auto key = Helper::stringToLower(std::get<std::string>(opcodeToken.value));
  std::optional<Opcode> opcode = parseMnemonic(key);

  if (!opcode) {
    addError(opcodeToken.span, "That opcode doesn't exist");
    return std::nullopt;
  }

  Token current = peek();
  std::vector<Operand> operands;
  while (peek().type != TokenType::NEWLINE) {

    std::optional<Operand> operand = parseOperand();
    if (!operand)
      return std::nullopt;

    operands.push_back(*operand);

    if (peek().type == TokenType::COMMA) {
      advance();
    } else if (peek().type != TokenType::NEWLINE) {
      addError(peek().span, "Expected comma between operands");
      return std::nullopt;
    }
  }

  Instruction result;
  result.operands = operands;
  result.opcode = *opcode;
  result.span = opcodeToken.span;

  return result;
}

FlatProgram Parser::parse() {
  FlatProgram program;

  while (!endOfFile()) {
    if (match(TokenType::NEWLINE)) {
      advance();
      continue;
    }

    // Data directive
    if (isDataDirective()) {
      auto dd = parseDataDirective();
      if (dd) {
        program.emplace_back(*dd);
      } else {
        // Error has been added, just skip to next line
        skipToNextLine();
      }
      // parseDataDirective has been eaten all token to the end of line
      if (match(TokenType::NEWLINE))
        advance();
      continue;
    }

    // Label declaration
    if (isLabelDeclaration()) {
      Label label = parseLabelDeclaration(); // consume name and ':'
      program.emplace_back(label);
      continue;
    }

    // Global directive
    if (isGlobalDirective()) {
      auto gd = parseGlobalDirective();
      if (gd) {
        program.emplace_back(*gd);
      } else {
        skipToNextLine();
      }
      if (match(TokenType::NEWLINE))
        advance();
      continue;
    }

    // Section directive
    if (isSectionDirective()) {
      auto sd = parseSectionDirective();
      if (sd) {
        program.emplace_back(*sd);
      } else {
        skipToNextLine();
      }
      if (match(TokenType::NEWLINE))
        advance();
      continue;
    }

    // equ directive
    if (isEquDirection()) {
      auto ed = parseEquDirective();
      if (ed) {
        program.emplace_back(*ed);
      } else {
        skipToNextLine();
      }
      if (match(TokenType::NEWLINE))
        advance();
      continue;
    }

    if (isInstruction()) {
      auto inst = parseInstruction();
      if (inst) {
        program.emplace_back(*inst);
      } else {
        skipToNextLine();
      }
      if (match(TokenType::NEWLINE))
        advance();
      continue;
    }
    addError(peek().span, "unexpected token");
    skipToNextLine();
  }

  return program;
}

Parser::Parser(std::vector<Token> tokens) : tokens(tokens), currentToken(0) {}

const std::vector<CompilerError> &Parser::getErrors() const noexcept {
  return errors;
}
bool Parser::hasErrors() const noexcept { return errors.size() > 0; }

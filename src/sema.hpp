#pragma once

#include <stdexcept>
#include <string>

#include "ast.hpp"
#include "symbol_table.hpp"

namespace minic {

struct SemanticError : std::runtime_error {
  SourceLoc loc{};
  explicit SemanticError(const SourceLoc& l, const std::string& msg)
      : std::runtime_error(msg), loc(l) {}
};

class SemanticAnalyzer {
public:
  void analyze(const Program& program);

private:
  SymbolTable sym_;

  void analyzeStmt(const Stmt& s);
  void analyzeBlock(const Block& b);
  TypeKind analyzeExpr(const Expr& e);

  [[noreturn]] void error(const SourceLoc& loc, const std::string& msg);
};

} // namespace minic


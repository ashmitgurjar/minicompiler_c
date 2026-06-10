#pragma once

#include <memory>
#include "ast.hpp"

namespace minic {

// Optimize expressions via constant folding
std::unique_ptr<Expr> optimizeExpr(std::unique_ptr<Expr> expr);

// Optimize statements recursively
std::unique_ptr<Stmt> optimizeStmt(std::unique_ptr<Stmt> stmt);

// Run constant folding optimizer pass over the entire program AST
void optimizeProgram(Program& program);

} // namespace minic

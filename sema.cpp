#include "sema.hpp"

#include <sstream>

namespace minic {

[[noreturn]] void SemanticAnalyzer::error(const SourceLoc& loc,
                                         const std::string& msg) {
  std::ostringstream oss;
  oss << "line " << loc.line << ":" << loc.col << ": " << msg;
  throw SemanticError(loc, oss.str());
}

void SemanticAnalyzer::analyze(const Program& program) {
  for (const auto& s : program.stmts) analyzeStmt(*s);
}

void SemanticAnalyzer::analyzeStmt(const Stmt& s) {
  if (auto* b = dynamic_cast<const Block*>(&s)) {
    analyzeBlock(*b);
    return;
  }
  if (auto* d = dynamic_cast<const Decl*>(&s)) {
    Symbol sym;
    sym.name = d->name;
    sym.type = d->type;
    sym.declLoc = d->loc;

    if (!sym_.declare(sym)) {
      error(d->loc, "redeclaration of '" + d->name + "'");
    }

    if (d->init) {
      auto t = analyzeExpr(*d->init);
      if (t != TypeKind::Int && t != TypeKind::Error) {
        error(d->loc, "unsupported initializer type for '" + d->name + "'");
      }
    }
    return;
  }
  if (auto* a = dynamic_cast<const Assign*>(&s)) {
    const Symbol* sym = sym_.lookup(a->name);
    if (!sym) error(a->loc, "use of undeclared identifier '" + a->name + "'");
    auto t = analyzeExpr(*a->value);
    if (sym && sym->type == TypeKind::Int && t == TypeKind::Error) {
      // keep going; error already reported upstream
      return;
    }
    if (sym && sym->type != t && t != TypeKind::Error) {
      error(a->loc, "type mismatch assigning to '" + a->name + "'");
    }
    return;
  }

  error(s.loc, "unknown statement");
}

void SemanticAnalyzer::analyzeBlock(const Block& b) {
  sym_.pushScope();
  for (const auto& s : b.stmts) analyzeStmt(*s);
  sym_.popScope();
}

TypeKind SemanticAnalyzer::analyzeExpr(const Expr& e) {
  if (auto* i = dynamic_cast<const IntLiteral*>(&e)) {
    (void)i;
    return TypeKind::Int;
  }
  if (auto* v = dynamic_cast<const VarRef*>(&e)) {
    const Symbol* sym = sym_.lookup(v->name);
    if (!sym) error(v->loc, "use of undeclared identifier '" + v->name + "'");
    return sym ? sym->type : TypeKind::Error;
  }
  if (auto* b = dynamic_cast<const BinaryExpr*>(&e)) {
    auto lt = analyzeExpr(*b->lhs);
    auto rt = analyzeExpr(*b->rhs);
    if (lt == TypeKind::Error || rt == TypeKind::Error) return TypeKind::Error;
    if (lt != TypeKind::Int || rt != TypeKind::Int) {
      error(b->loc, "binary operator expects int operands");
    }
    return TypeKind::Int;
  }
  error(e.loc, "unknown expression");
}

} // namespace minic


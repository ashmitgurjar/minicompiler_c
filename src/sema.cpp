#include "sema.hpp"

#include <sstream>

namespace minic {
namespace {

static std::string typeName(TypeKind type) {
  switch (type) {
    case TypeKind::Int: return "int";
    case TypeKind::Float: return "float";
    case TypeKind::Bool: return "bool";
    case TypeKind::String: return "string";
    case TypeKind::Error: return "error";
  }
  return "unknown";
}

bool areTypesCompatible(TypeKind expected, TypeKind actual) {
  if (expected == actual) return true;
  // Allow implicit coercion from Int to Float
  if (expected == TypeKind::Float && actual == TypeKind::Int) return true;
  return false;
}

bool isNumeric(TypeKind type) {
  return type == TypeKind::Int || type == TypeKind::Float;
}

} // namespace

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
      if (t == TypeKind::Error) return;
      if (!areTypesCompatible(d->type, t)) {
        error(d->loc, "type mismatch in initializer for '" + d->name +
                      "': cannot initialize " + typeName(d->type) +
                      " with " + typeName(t));
      }
    }
    return;
  }
  if (auto* a = dynamic_cast<const Assign*>(&s)) {
    const Symbol* sym = sym_.lookup(a->name);
    if (!sym) {
      error(a->loc, "use of undeclared identifier '" + a->name + "'");
    }
    auto t = analyzeExpr(*a->value);
    if (t == TypeKind::Error) return;
    if (!areTypesCompatible(sym->type, t)) {
      error(a->loc, "type mismatch assigning to '" + a->name +
                    "': expected " + typeName(sym->type) +
                    ", got " + typeName(t));
    }
    return;
  }
  if (auto* iff = dynamic_cast<const If*>(&s)) {
    auto condType = analyzeExpr(*iff->cond);
    if (condType != TypeKind::Bool && condType != TypeKind::Error) {
      error(iff->cond->loc, "if condition must be bool, got " + typeName(condType));
    }
    analyzeStmt(*iff->thenStmt);
    if (iff->elseStmt) {
      analyzeStmt(*iff->elseStmt);
    }
    return;
  }
  if (auto* w = dynamic_cast<const While*>(&s)) {
    auto condType = analyzeExpr(*w->cond);
    if (condType != TypeKind::Bool && condType != TypeKind::Error) {
      error(w->cond->loc, "while condition must be bool, got " + typeName(condType));
    }
    analyzeStmt(*w->body);
    return;
  }
  if (auto* p = dynamic_cast<const Print*>(&s)) {
    (void)analyzeExpr(*p->expr);
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
  if (dynamic_cast<const IntLiteral*>(&e)) {
    return TypeKind::Int;
  }
  if (dynamic_cast<const FloatLiteral*>(&e)) {
    return TypeKind::Float;
  }
  if (dynamic_cast<const BoolLiteral*>(&e)) {
    return TypeKind::Bool;
  }
  if (dynamic_cast<const StringLiteral*>(&e)) {
    return TypeKind::String;
  }
  if (auto* v = dynamic_cast<const VarRef*>(&e)) {
    const Symbol* sym = sym_.lookup(v->name);
    if (!sym) {
      error(v->loc, "use of undeclared identifier '" + v->name + "'");
    }
    return sym ? sym->type : TypeKind::Error;
  }
  if (auto* u = dynamic_cast<const UnaryExpr*>(&e)) {
    auto t = analyzeExpr(*u->expr);
    if (t == TypeKind::Error) return TypeKind::Error;
    if (u->op == UnaryOp::Neg) {
      if (!isNumeric(t)) {
        error(u->loc, "unary minus expects int or float operand, got " + typeName(t));
      }
      return t;
    }
    if (u->op == UnaryOp::Not) {
      if (t != TypeKind::Bool) {
        error(u->loc, "logical not expects bool operand, got " + typeName(t));
      }
      return TypeKind::Bool;
    }
    return TypeKind::Error;
  }
  if (auto* b = dynamic_cast<const BinaryExpr*>(&e)) {
    auto lt = analyzeExpr(*b->lhs);
    auto rt = analyzeExpr(*b->rhs);
    if (lt == TypeKind::Error || rt == TypeKind::Error) return TypeKind::Error;

    switch (b->op) {
      case BinOp::Add:
      case BinOp::Sub:
      case BinOp::Mul:
      case BinOp::Div: {
        if (!isNumeric(lt) || !isNumeric(rt)) {
          error(b->loc, "arithmetic operator expects numeric operands, got " +
                        typeName(lt) + " and " + typeName(rt));
        }
        if (lt == TypeKind::Float || rt == TypeKind::Float) {
          return TypeKind::Float;
        }
        return TypeKind::Int;
      }
      case BinOp::Lt:
      case BinOp::Le:
      case BinOp::Gt:
      case BinOp::Ge: {
        if (!isNumeric(lt) || !isNumeric(rt)) {
          error(b->loc, "comparison operator expects numeric operands, got " +
                        typeName(lt) + " and " + typeName(rt));
        }
        return TypeKind::Bool;
      }
      case BinOp::Eq:
      case BinOp::Ne: {
        if (isNumeric(lt) && isNumeric(rt)) {
          return TypeKind::Bool;
        }
        if (lt == rt) {
          return TypeKind::Bool;
        }
        error(b->loc, "equality operator expects compatible operands, got " +
                      typeName(lt) + " and " + typeName(rt));
      }
      case BinOp::And:
      case BinOp::Or: {
        if (lt != TypeKind::Bool || rt != TypeKind::Bool) {
          error(b->loc, "logical operator expects bool operands, got " +
                        typeName(lt) + " and " + typeName(rt));
        }
        return TypeKind::Bool;
      }
    }
    return TypeKind::Error;
  }

  error(e.loc, "unknown expression");
}

} // namespace minic

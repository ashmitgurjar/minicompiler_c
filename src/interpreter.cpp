#include "interpreter.hpp"

#include <iostream>
#include <sstream>

namespace minic {

std::string valToString(const Val& v) {
  if (std::holds_alternative<int64_t>(v)) {
    return std::to_string(std::get<int64_t>(v));
  }
  if (std::holds_alternative<double>(v)) {
    std::string s = std::to_string(std::get<double>(v));
    // Strip trailing zeros for prettier output
    if (s.find('.') != std::string::npos) {
      while (s.back() == '0') s.pop_back();
      if (s.back() == '.') s.pop_back();
    }
    return s;
  }
  if (std::holds_alternative<bool>(v)) {
    return std::get<bool>(v) ? "true" : "false";
  }
  return std::get<std::string>(v);
}

InterpreterEnv::InterpreterEnv() {
  pushScope();
}

void InterpreterEnv::pushScope() {
  scopes_.emplace_back();
}

void InterpreterEnv::popScope() {
  if (scopes_.empty()) {
    throw std::runtime_error("popScope called on empty interpreter environment");
  }
  scopes_.pop_back();
}

void InterpreterEnv::declare(const std::string& name, const Val& val) {
  if (scopes_.empty()) pushScope();
  scopes_.back()[name] = val;
}

void InterpreterEnv::assign(const std::string& name, const Val& val) {
  for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
    auto entry = it->find(name);
    if (entry != it->end()) {
      // Coerce Int value if assigning to double
      if (std::holds_alternative<double>(entry->second) && std::holds_alternative<int64_t>(val)) {
        entry->second = static_cast<double>(std::get<int64_t>(val));
      } else {
        entry->second = val;
      }
      return;
    }
  }
  throw std::runtime_error("undefined variable in interpreter assignment: " + name);
}

Val InterpreterEnv::get(const std::string& name) const {
  for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
    auto entry = it->find(name);
    if (entry != it->end()) {
      return entry->second;
    }
  }
  throw std::runtime_error("undefined variable in interpreter lookup: " + name);
}

[[noreturn]] void Interpreter::error(const SourceLoc& loc, const std::string& msg) {
  std::ostringstream oss;
  oss << "runtime error at line " << loc.line << ":" << loc.col << ": " << msg;
  throw RuntimeError(loc, oss.str());
}

void Interpreter::run(const Program& program) {
  for (const auto& s : program.stmts) {
    executeStmt(*s);
  }
}

void Interpreter::executeStmt(const Stmt& s) {
  if (auto* b = dynamic_cast<const Block*>(&s)) {
    env_.pushScope();
    for (const auto& st : b->stmts) {
      executeStmt(*st);
    }
    env_.popScope();
    return;
  }
  if (auto* d = dynamic_cast<const Decl*>(&s)) {
    Val initVal;
    switch (d->type) {
      case TypeKind::Int: initVal = int64_t{0}; break;
      case TypeKind::Float: initVal = double{0.0}; break;
      case TypeKind::Bool: initVal = false; break;
      case TypeKind::String: initVal = std::string{""}; break;
      case TypeKind::Error: initVal = false; break;
    }

    if (d->init) {
      initVal = evalExpr(*d->init);
      // Coerce standard numeric type if declared float and init is int
      if (d->type == TypeKind::Float && std::holds_alternative<int64_t>(initVal)) {
        initVal = static_cast<double>(std::get<int64_t>(initVal));
      }
    }

    env_.declare(d->name, initVal);
    return;
  }
  if (auto* a = dynamic_cast<const Assign*>(&s)) {
    Val val = evalExpr(*a->value);
    env_.assign(a->name, val);
    return;
  }
  if (auto* iff = dynamic_cast<const If*>(&s)) {
    Val cond = evalExpr(*iff->cond);
    if (std::get<bool>(cond)) {
      executeStmt(*iff->thenStmt);
    } else if (iff->elseStmt) {
      executeStmt(*iff->elseStmt);
    }
    return;
  }
  if (auto* w = dynamic_cast<const While*>(&s)) {
    while (true) {
      Val cond = evalExpr(*w->cond);
      if (!std::get<bool>(cond)) break;
      executeStmt(*w->body);
    }
    return;
  }
  if (auto* p = dynamic_cast<const Print*>(&s)) {
    Val val = evalExpr(*p->expr);
    std::cout << valToString(val) << "\n";
    return;
  }

  error(s.loc, "unknown statement during execution");
}

Val Interpreter::evalExpr(const Expr& e) {
  if (auto* i = dynamic_cast<const IntLiteral*>(&e)) {
    return i->value;
  }
  if (auto* f = dynamic_cast<const FloatLiteral*>(&e)) {
    return f->value;
  }
  if (auto* b = dynamic_cast<const BoolLiteral*>(&e)) {
    return b->value;
  }
  if (auto* s = dynamic_cast<const StringLiteral*>(&e)) {
    return s->value;
  }
  if (auto* v = dynamic_cast<const VarRef*>(&e)) {
    return env_.get(v->name);
  }
  if (auto* u = dynamic_cast<const UnaryExpr*>(&e)) {
    Val val = evalExpr(*u->expr);
    if (u->op == UnaryOp::Neg) {
      if (std::holds_alternative<int64_t>(val)) {
        return -std::get<int64_t>(val);
      }
      return -std::get<double>(val);
    }
    if (u->op == UnaryOp::Not) {
      return !std::get<bool>(val);
    }
  }
  if (auto* b = dynamic_cast<const BinaryExpr*>(&e)) {
    Val l = evalExpr(*b->lhs);
    Val r = evalExpr(*b->rhs);

    // Helper check to see if numeric variables are mixed
    bool lIsInt = std::holds_alternative<int64_t>(l);
    bool rIsInt = std::holds_alternative<int64_t>(r);

    switch (b->op) {
      case BinOp::Add:
        if (lIsInt && rIsInt) return std::get<int64_t>(l) + std::get<int64_t>(r);
        return (lIsInt ? static_cast<double>(std::get<int64_t>(l)) : std::get<double>(l)) +
               (rIsInt ? static_cast<double>(std::get<int64_t>(r)) : std::get<double>(r));
      case BinOp::Sub:
        if (lIsInt && rIsInt) return std::get<int64_t>(l) - std::get<int64_t>(r);
        return (lIsInt ? static_cast<double>(std::get<int64_t>(l)) : std::get<double>(l)) -
               (rIsInt ? static_cast<double>(std::get<int64_t>(r)) : std::get<double>(r));
      case BinOp::Mul:
        if (lIsInt && rIsInt) return std::get<int64_t>(l) * std::get<int64_t>(r);
        return (lIsInt ? static_cast<double>(std::get<int64_t>(l)) : std::get<double>(l)) *
               (rIsInt ? static_cast<double>(std::get<int64_t>(r)) : std::get<double>(r));
      case BinOp::Div:
        if (lIsInt && rIsInt) {
          int64_t denom = std::get<int64_t>(r);
          if (denom == 0) error(b->loc, "division by zero");
          return std::get<int64_t>(l) / denom;
        } else {
          double denom = rIsInt ? static_cast<double>(std::get<int64_t>(r)) : std::get<double>(r);
          if (denom == 0.0) error(b->loc, "division by zero");
          return (lIsInt ? static_cast<double>(std::get<int64_t>(l)) : std::get<double>(l)) / denom;
        }
      case BinOp::Eq:
        if (lIsInt && rIsInt) return std::get<int64_t>(l) == std::get<int64_t>(r);
        if (std::holds_alternative<double>(l) || std::holds_alternative<double>(r)) {
          double valL = lIsInt ? static_cast<double>(std::get<int64_t>(l)) : std::get<double>(l);
          double valR = rIsInt ? static_cast<double>(std::get<int64_t>(r)) : std::get<double>(r);
          return valL == valR;
        }
        if (std::holds_alternative<bool>(l)) return std::get<bool>(l) == std::get<bool>(r);
        return std::get<std::string>(l) == std::get<std::string>(r);
      case BinOp::Ne:
        if (lIsInt && rIsInt) return std::get<int64_t>(l) != std::get<int64_t>(r);
        if (std::holds_alternative<double>(l) || std::holds_alternative<double>(r)) {
          double valL = lIsInt ? static_cast<double>(std::get<int64_t>(l)) : std::get<double>(l);
          double valR = rIsInt ? static_cast<double>(std::get<int64_t>(r)) : std::get<double>(r);
          return valL != valR;
        }
        if (std::holds_alternative<bool>(l)) return std::get<bool>(l) != std::get<bool>(r);
        return std::get<std::string>(l) != std::get<std::string>(r);
      case BinOp::Lt: {
        double valL = lIsInt ? static_cast<double>(std::get<int64_t>(l)) : std::get<double>(l);
        double valR = rIsInt ? static_cast<double>(std::get<int64_t>(r)) : std::get<double>(r);
        return valL < valR;
      }
      case BinOp::Le: {
        double valL = lIsInt ? static_cast<double>(std::get<int64_t>(l)) : std::get<double>(l);
        double valR = rIsInt ? static_cast<double>(std::get<int64_t>(r)) : std::get<double>(r);
        return valL <= valR;
      }
      case BinOp::Gt: {
        double valL = lIsInt ? static_cast<double>(std::get<int64_t>(l)) : std::get<double>(l);
        double valR = rIsInt ? static_cast<double>(std::get<int64_t>(r)) : std::get<double>(r);
        return valL > valR;
      }
      case BinOp::Ge: {
        double valL = lIsInt ? static_cast<double>(std::get<int64_t>(l)) : std::get<double>(l);
        double valR = rIsInt ? static_cast<double>(std::get<int64_t>(r)) : std::get<double>(r);
        return valL >= valR;
      }
      case BinOp::And:
        return std::get<bool>(l) && std::get<bool>(r);
      case BinOp::Or:
        return std::get<bool>(l) || std::get<bool>(r);
    }
  }

  error(e.loc, "unknown expression during evaluation");
}

} // namespace minic

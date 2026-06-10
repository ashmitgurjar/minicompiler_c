#include "optimizer.hpp"
#include <iostream>

namespace minic {

std::unique_ptr<Expr> optimizeExpr(std::unique_ptr<Expr> expr) {
  if (!expr) return nullptr;

  if (auto* bin = dynamic_cast<BinaryExpr*>(expr.get())) {
    bin->lhs = optimizeExpr(std::move(bin->lhs));
    bin->rhs = optimizeExpr(std::move(bin->rhs));

    // Case 1: Both operands are Int Literals -> Fold!
    auto* lhsInt = dynamic_cast<IntLiteral*>(bin->lhs.get());
    auto* rhsInt = dynamic_cast<IntLiteral*>(bin->rhs.get());
    if (lhsInt && rhsInt) {
      int64_t val = 0;
      bool isRel = false;
      bool relVal = false;

      switch (bin->op) {
        case BinOp::Add: val = lhsInt->value + rhsInt->value; break;
        case BinOp::Sub: val = lhsInt->value - rhsInt->value; break;
        case BinOp::Mul: val = lhsInt->value * rhsInt->value; break;
        case BinOp::Div: val = rhsInt->value != 0 ? lhsInt->value / rhsInt->value : 0; break;
        
        case BinOp::Eq: relVal = (lhsInt->value == rhsInt->value); isRel = true; break;
        case BinOp::Ne: relVal = (lhsInt->value != rhsInt->value); isRel = true; break;
        case BinOp::Lt: relVal = (lhsInt->value < rhsInt->value); isRel = true; break;
        case BinOp::Le: relVal = (lhsInt->value <= rhsInt->value); isRel = true; break;
        case BinOp::Gt: relVal = (lhsInt->value > rhsInt->value); isRel = true; break;
        case BinOp::Ge: relVal = (lhsInt->value >= rhsInt->value); isRel = true; break;
        default: break;
      }

      if (isRel) {
        auto folded = std::make_unique<BoolLiteral>(relVal);
        folded->loc = bin->loc;
        return folded;
      } else {
        auto folded = std::make_unique<IntLiteral>(val);
        folded->loc = bin->loc;
        return folded;
      }
    }

    // Case 2: Both operands are Float Literals -> Fold!
    auto* lhsFloat = dynamic_cast<FloatLiteral*>(bin->lhs.get());
    auto* rhsFloat = dynamic_cast<FloatLiteral*>(bin->rhs.get());
    if (lhsFloat && rhsFloat) {
      double val = 0.0;
      bool isRel = false;
      bool relVal = false;

      switch (bin->op) {
        case BinOp::Add: val = lhsFloat->value + rhsFloat->value; break;
        case BinOp::Sub: val = lhsFloat->value - rhsFloat->value; break;
        case BinOp::Mul: val = lhsFloat->value * rhsFloat->value; break;
        case BinOp::Div: val = rhsFloat->value != 0.0 ? lhsFloat->value / rhsFloat->value : 0.0; break;
        
        case BinOp::Eq: relVal = (lhsFloat->value == rhsFloat->value); isRel = true; break;
        case BinOp::Ne: relVal = (lhsFloat->value != rhsFloat->value); isRel = true; break;
        case BinOp::Lt: relVal = (lhsFloat->value < rhsFloat->value); isRel = true; break;
        case BinOp::Le: relVal = (lhsFloat->value <= rhsFloat->value); isRel = true; break;
        case BinOp::Gt: relVal = (lhsFloat->value > rhsFloat->value); isRel = true; break;
        case BinOp::Ge: relVal = (lhsFloat->value >= rhsFloat->value); isRel = true; break;
        default: break;
      }

      if (isRel) {
        auto folded = std::make_unique<BoolLiteral>(relVal);
        folded->loc = bin->loc;
        return folded;
      } else {
        auto folded = std::make_unique<FloatLiteral>(val);
        folded->loc = bin->loc;
        return folded;
      }
    }

    // Case 3: Both operands are Bool Literals -> Fold AND / OR!
    auto* lhsBool = dynamic_cast<BoolLiteral*>(bin->lhs.get());
    auto* rhsBool = dynamic_cast<BoolLiteral*>(bin->rhs.get());
    if (lhsBool && rhsBool) {
      bool relVal = false;
      bool isRel = false;
      switch (bin->op) {
        case BinOp::And: relVal = lhsBool->value && rhsBool->value; isRel = true; break;
        case BinOp::Or:  relVal = lhsBool->value || rhsBool->value; isRel = true; break;
        case BinOp::Eq:  relVal = lhsBool->value == rhsBool->value; isRel = true; break;
        case BinOp::Ne:  relVal = lhsBool->value != rhsBool->value; isRel = true; break;
        default: break;
      }
      if (isRel) {
        auto folded = std::make_unique<BoolLiteral>(relVal);
        folded->loc = bin->loc;
        return folded;
      }
    }
  }

  if (auto* un = dynamic_cast<UnaryExpr*>(expr.get())) {
    un->expr = optimizeExpr(std::move(un->expr));

    // Fold Negated Int
    if (un->op == UnaryOp::Neg) {
      if (auto* valInt = dynamic_cast<IntLiteral*>(un->expr.get())) {
        auto folded = std::make_unique<IntLiteral>(-valInt->value);
        folded->loc = un->loc;
        return folded;
      }
      if (auto* valFloat = dynamic_cast<FloatLiteral*>(un->expr.get())) {
        auto folded = std::make_unique<FloatLiteral>(-valFloat->value);
        folded->loc = un->loc;
        return folded;
      }
    }

    // Fold Not Boolean
    if (un->op == UnaryOp::Not) {
      if (auto* valBool = dynamic_cast<BoolLiteral*>(un->expr.get())) {
        auto folded = std::make_unique<BoolLiteral>(!valBool->value);
        folded->loc = un->loc;
        return folded;
      }
    }
  }

  return expr;
}

std::unique_ptr<Stmt> optimizeStmt(std::unique_ptr<Stmt> stmt) {
  if (!stmt) return nullptr;

  if (auto* d = dynamic_cast<Decl*>(stmt.get())) {
    if (d->init) {
      d->init = optimizeExpr(std::move(d->init));
    }
  } else if (auto* a = dynamic_cast<Assign*>(stmt.get())) {
    a->value = optimizeExpr(std::move(a->value));
  } else if (auto* p = dynamic_cast<Print*>(stmt.get())) {
    p->expr = optimizeExpr(std::move(p->expr));
  } else if (auto* b = dynamic_cast<Block*>(stmt.get())) {
    for (auto& s : b->stmts) {
      s = optimizeStmt(std::move(s));
    }
  } else if (auto* iff = dynamic_cast<If*>(stmt.get())) {
    iff->cond = optimizeExpr(std::move(iff->cond));
    iff->thenStmt = optimizeStmt(std::move(iff->thenStmt));
    if (iff->elseStmt) {
      iff->elseStmt = optimizeStmt(std::move(iff->elseStmt));
    }
  } else if (auto* w = dynamic_cast<While*>(stmt.get())) {
    w->cond = optimizeExpr(std::move(w->cond));
    w->body = optimizeStmt(std::move(w->body));
  }

  return stmt;
}

void optimizeProgram(Program& program) {
  for (auto& s : program.stmts) {
    s = optimizeStmt(std::move(s));
  }
}

} // namespace minic

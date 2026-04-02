#include "print.hpp"

#include <ostream>
#include <string>

namespace minic {
namespace {

struct Indent {
  int n = 0;
  void inc() { n += 2; }
  void dec() { n -= 2; }
  void pad(std::ostream& os) const { os << std::string(static_cast<size_t>(n), ' '); }
};

const char* binOpName(BinOp op) {
  switch (op) {
    case BinOp::Add: return "+";
    case BinOp::Sub: return "-";
    case BinOp::Mul: return "*";
    case BinOp::Div: return "/";
  }
  return "?";
}

void printExpr(std::ostream& os, const Expr& e, Indent in);
void printStmt(std::ostream& os, const Stmt& s, Indent in);

void printExpr(std::ostream& os, const Expr& e, Indent in) {
  if (auto* i = dynamic_cast<const IntLiteral*>(&e)) {
    in.pad(os);
    os << "IntLiteral(" << i->value << ")\n";
    return;
  }
  if (auto* v = dynamic_cast<const VarRef*>(&e)) {
    in.pad(os);
    os << "VarRef(" << v->name << ")\n";
    return;
  }
  if (auto* b = dynamic_cast<const BinaryExpr*>(&e)) {
    in.pad(os);
    os << "BinaryExpr(" << binOpName(b->op) << ")\n";
    in.inc();
    in.pad(os);
    os << "lhs:\n";
    in.inc();
    printExpr(os, *b->lhs, in);
    in.dec();
    in.pad(os);
    os << "rhs:\n";
    in.inc();
    printExpr(os, *b->rhs, in);
    in.dec();
    in.dec();
    return;
  }
  in.pad(os);
  os << "Expr(?)\n";
}

void printStmt(std::ostream& os, const Stmt& s, Indent in) {
  if (auto* b = dynamic_cast<const Block*>(&s)) {
    in.pad(os);
    os << "Block\n";
    in.inc();
    for (const auto& st : b->stmts) printStmt(os, *st, in);
    return;
  }
  if (auto* d = dynamic_cast<const Decl*>(&s)) {
    in.pad(os);
    os << "Decl(int " << d->name << ")\n";
    if (d->init) {
      in.inc();
      in.pad(os);
      os << "init:\n";
      in.inc();
      printExpr(os, *d->init, in);
      in.dec();
      in.dec();
    }
    return;
  }
  if (auto* a = dynamic_cast<const Assign*>(&s)) {
    in.pad(os);
    os << "Assign(" << a->name << ")\n";
    in.inc();
    printExpr(os, *a->value, in);
    return;
  }
  in.pad(os);
  os << "Stmt(?)\n";
}

} // namespace

void printAST(std::ostream& os, const Program& p) {
  Indent in;
  os << "Program\n";
  in.inc();
  for (const auto& s : p.stmts) printStmt(os, *s, in);
}

} // namespace minic


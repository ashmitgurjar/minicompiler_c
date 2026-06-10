#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace minic {

struct SourceLoc {
  int line = 1;
  int col = 1;
};

enum class TypeKind { Int, Float, Bool, String, Error };

struct Node {
  SourceLoc loc{};
  virtual ~Node() = default;
};

struct Expr : Node {
  virtual ~Expr() = default;
};

struct Stmt : Node {
  virtual ~Stmt() = default;
};

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

struct IntLiteral : Expr {
  int64_t value = 0;
  explicit IntLiteral(int64_t v) : value(v) {}
};

struct FloatLiteral : Expr {
  double value = 0.0;
  explicit FloatLiteral(double v) : value(v) {}
};

struct BoolLiteral : Expr {
  bool value = false;
  explicit BoolLiteral(bool v) : value(v) {}
};

struct StringLiteral : Expr {
  std::string value;
  explicit StringLiteral(std::string v) : value(std::move(v)) {}
};

struct VarRef : Expr {
  std::string name;
  explicit VarRef(std::string n) : name(std::move(n)) {}
};

enum class BinOp {
  Add, Sub, Mul, Div,
  Eq, Ne, Lt, Le, Gt, Ge,
  And, Or
};

struct BinaryExpr : Expr {
  BinOp op;
  ExprPtr lhs;
  ExprPtr rhs;
  BinaryExpr(BinOp o, ExprPtr a, ExprPtr b)
      : op(o), lhs(std::move(a)), rhs(std::move(b)) {}
};

enum class UnaryOp { Neg, Not };

struct UnaryExpr : Expr {
  UnaryOp op;
  ExprPtr expr;
  UnaryExpr(UnaryOp o, ExprPtr e) : op(o), expr(std::move(e)) {}
};

struct Decl : Stmt {
  std::string name;
  TypeKind type = TypeKind::Int;
  ExprPtr init; // optional
  Decl(std::string n, TypeKind t, ExprPtr i)
      : name(std::move(n)), type(t), init(std::move(i)) {}
};

struct Assign : Stmt {
  std::string name;
  ExprPtr value;
  Assign(std::string n, ExprPtr v) : name(std::move(n)), value(std::move(v)) {}
};

struct Block : Stmt {
  std::vector<StmtPtr> stmts;
};

struct If : Stmt {
  ExprPtr cond;
  StmtPtr thenStmt;
  StmtPtr elseStmt; // optional, can be null
  If(ExprPtr c, StmtPtr t, StmtPtr e)
      : cond(std::move(c)), thenStmt(std::move(t)), elseStmt(std::move(e)) {}
};

struct While : Stmt {
  ExprPtr cond;
  StmtPtr body;
  While(ExprPtr c, StmtPtr b) : cond(std::move(c)), body(std::move(b)) {}
};

struct Print : Stmt {
  ExprPtr expr;
  explicit Print(ExprPtr e) : expr(std::move(e)) {}
};

struct Program : Node {
  std::vector<StmtPtr> stmts;
};

} // namespace minic

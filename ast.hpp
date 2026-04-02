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

enum class TypeKind { Int, Error };

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

struct VarRef : Expr {
  std::string name;
  explicit VarRef(std::string n) : name(std::move(n)) {}
};

enum class BinOp { Add, Sub, Mul, Div };

struct BinaryExpr : Expr {
  BinOp op;
  ExprPtr lhs;
  ExprPtr rhs;
  BinaryExpr(BinOp o, ExprPtr a, ExprPtr b)
      : op(o), lhs(std::move(a)), rhs(std::move(b)) {}
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

struct Program : Node {
  std::vector<StmtPtr> stmts;
};

} // namespace minic


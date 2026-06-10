#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <stdexcept>

#include "ast.hpp"

namespace minic {

// A runtime value in the Mini-C interpreter
using Val = std::variant<int64_t, double, bool, std::string>;

std::string valToString(const Val& v);

class InterpreterEnv {
public:
  InterpreterEnv();

  void pushScope();
  void popScope();

  void declare(const std::string& name, const Val& val);
  void assign(const std::string& name, const Val& val);
  Val get(const std::string& name) const;

private:
  std::vector<std::unordered_map<std::string, Val>> scopes_;
};

class RuntimeError : public std::runtime_error {
public:
  SourceLoc loc{};
  explicit RuntimeError(const SourceLoc& l, const std::string& msg)
      : std::runtime_error(msg), loc(l) {}
};

class Interpreter {
public:
  void run(const Program& program);

private:
  InterpreterEnv env_;

  void executeStmt(const Stmt& s);
  Val evalExpr(const Expr& e);

  [[noreturn]] void error(const SourceLoc& loc, const std::string& msg);
};

} // namespace minic

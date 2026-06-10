#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ast.hpp"

namespace minic {

struct Symbol {
  std::string name;
  TypeKind type = TypeKind::Int;
  SourceLoc declLoc{};
};

class SymbolTable {
public:
  SymbolTable();

  void pushScope();
  void popScope();

  // Returns false if name already exists in current scope.
  bool declare(const Symbol& sym);

  // Lookup in nearest enclosing scope.
  const Symbol* lookup(const std::string& name) const;

  // Lookup only in current scope.
  const Symbol* lookupCurrent(const std::string& name) const;

  int depth() const;

private:
  using Scope = std::unordered_map<std::string, Symbol>;
  std::vector<Scope> scopes_;
};

} // namespace minic


#include "symbol_table.hpp"

#include <stdexcept>

namespace minic {

SymbolTable::SymbolTable() { pushScope(); }

void SymbolTable::pushScope() { scopes_.emplace_back(); }

void SymbolTable::popScope() {
  if (scopes_.empty()) throw std::runtime_error("popScope on empty symbol table");
  scopes_.pop_back();
}

bool SymbolTable::declare(const Symbol& sym) {
  if (scopes_.empty()) pushScope();
  auto& scope = scopes_.back();
  if (scope.find(sym.name) != scope.end()) return false;
  scope.emplace(sym.name, sym);
  return true;
}

const Symbol* SymbolTable::lookup(const std::string& name) const {
  for (int i = static_cast<int>(scopes_.size()) - 1; i >= 0; --i) {
    const auto& scope = scopes_[static_cast<size_t>(i)];
    auto it = scope.find(name);
    if (it != scope.end()) return &it->second;
  }
  return nullptr;
}

const Symbol* SymbolTable::lookupCurrent(const std::string& name) const {
  if (scopes_.empty()) return nullptr;
  const auto& scope = scopes_.back();
  auto it = scope.find(name);
  if (it != scope.end()) return &it->second;
  return nullptr;
}

int SymbolTable::depth() const { return static_cast<int>(scopes_.size()); }

} // namespace minic


// symbol_tree.hpp
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <parser/nodes.hpp>

namespace parser::sema::symbols {

struct SymbolInfo {
  enum class SymbolKind {
    kVariable,
    kFunction,
    kType
  };

  explicit SymbolInfo(std::string n, SymbolKind k, TypeVariant t)
      : name(std::move(n)), kind(k), type(std::move(t)) {}

  std::string name;
  SymbolKind kind;
  TypeVariant type;
};

struct Scope {
  Scope* parent = nullptr;
  std::vector<std::unique_ptr<Scope>> children;
  std::unordered_map<std::string, SymbolInfo> symbols;

  Scope* AddChild() {
    children.push_back(std::make_unique<Scope>());
    children.back()->parent = this;
    return children.back().get();
  }

  bool Insert(const std::string& name, SymbolInfo::SymbolKind kind, const TypeVariant& type) {
    auto [it, inserted] = symbols.try_emplace(name, name, kind, type);
    return inserted;
  }

  SymbolInfo* Lookup(const std::string& name) {
    for (Scope* s = this; s; s = s->parent) {
      auto it = s->symbols.find(name);
      if (it != s->symbols.end()) {
        return &it->second;
      }
    }
    return nullptr;
  }

  const SymbolInfo* Lookup(const std::string& name) const {
    for (const Scope* s = this; s; s = s->parent) {
      auto it = s->symbols.find(name);
      if (it != s->symbols.end()) {
        return &it->second;
      }
    }
    return nullptr;
  }

  SymbolInfo* LookupCurrent(const std::string& name) {
    auto it = symbols.find(name);
    return it != symbols.end() ? &it->second : nullptr;
  }

  const SymbolInfo* LookupCurrent(const std::string& name) const {
    auto it = symbols.find(name);
    return it != symbols.end() ? &it->second : nullptr;
  }
};

} // namespace parser::symbols
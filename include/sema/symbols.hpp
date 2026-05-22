// symbol_tree.hpp
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <parser/types.hpp>

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
  // ==========================================================================
  //  Methods
  // ==========================================================================
  Scope* AddChild();
  bool Insert(const std::string& name, 
              SymbolInfo::SymbolKind kind, 
              const TypeVariant& type);
  SymbolInfo* Lookup(const std::string& name);
  const SymbolInfo* Lookup(const std::string& name) const;
  SymbolInfo* LookupCurrent(const std::string& name);
  const SymbolInfo* LookupCurrent(const std::string& name) const;

  // ==========================================================================
  //  Data
  // ==========================================================================
  Scope* parent = nullptr;
  std::vector<std::unique_ptr<Scope>> children;
  std::unordered_map<std::string, SymbolInfo> symbols;
};

} // namespace parser::symbols
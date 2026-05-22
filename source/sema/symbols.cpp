#include <sema/symbols.hpp>

namespace parser::sema::symbols {

Scope* Scope::AddChild() {
    children.push_back(std::make_unique<Scope>());
    children.back()->parent = this;
    return children.back().get();
  }

  bool Scope::Insert(const std::string& name, SymbolInfo::SymbolKind kind, const TypeVariant& type) {
    auto [it, inserted] = symbols.try_emplace(name, name, kind, type);
    return inserted;
  }

  SymbolInfo* Scope::Lookup(const std::string& name) {
    for (Scope* s = this; s; s = s->parent) {
      if (auto it = s->LookupCurrent(name)) {
        return it;
      }
    }
    return nullptr;
  }

  const SymbolInfo* Scope::Lookup(const std::string& name) const {
    for (const Scope* s = this; s; s = s->parent) {
      if (auto it = s->LookupCurrent(name)) {
        return it;
      }
    }
    return nullptr;
  }

  SymbolInfo* Scope::LookupCurrent(const std::string& name) {
    auto it = symbols.find(name);
    return it != symbols.end() ? &it->second : nullptr;
  }

  const SymbolInfo* Scope::LookupCurrent(const std::string& name) const {
    auto it = symbols.find(name);
    return it != symbols.end() ? &it->second : nullptr;
  }

} // namespace parser::sema::symbols
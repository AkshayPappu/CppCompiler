#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace cc {

class Symbol;

class Scope {
public:
    explicit Scope(Scope* parent = nullptr) : parent_(parent) {}

    Scope* parent() const { return parent_; }

    // Returns false if the name already exists in this scope.
    bool declare(const std::string& name, Symbol* symbol) {
        auto [it, inserted] = symbols_.emplace(name, symbol);
        return inserted;
    }

    Symbol* lookupLocal(const std::string& name) const {
        auto it = symbols_.find(name);
        return it != symbols_.end() ? it->second : nullptr;
    }

    // Walks up the scope chain until the symbol is found or we run out of scopes.
    Symbol* lookup(const std::string& name) const {
        if (auto* sym = lookupLocal(name)) return sym;
        if (parent_) return parent_->lookup(name);
        return nullptr;
    }

    const std::unordered_map<std::string, Symbol*>& symbols() const {
        return symbols_;
    }

private:
    Scope* parent_;
    std::unordered_map<std::string, Symbol*> symbols_;
};

} // namespace cc

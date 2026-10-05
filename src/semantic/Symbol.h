#pragma once

#include "common/SourceLocation.h"
#include "semantic/Type.h"
#include <string>
#include <vector>

namespace cc {

enum class SymbolKind {
    Variable,
    Parameter,
    Function
};

class Symbol {
public:
    virtual ~Symbol() = default;

    SymbolKind symbolKind() const { return kind_; }
    const std::string& name() const { return name_; }
    const Type* type() const { return type_; }
    const SourceLocation& location() const { return location_; }

    const char* kindString() const {
        switch (kind_) {
            case SymbolKind::Variable:  return "variable";
            case SymbolKind::Parameter: return "parameter";
            case SymbolKind::Function:  return "function";
        }
        return "unknown";
    }

protected:
    Symbol(SymbolKind kind, std::string name, const Type* type,
           SourceLocation loc)
        : kind_(kind), name_(std::move(name)), type_(type),
          location_(std::move(loc)) {}

    SymbolKind kind_;
    std::string name_;
    const Type* type_;
    SourceLocation location_;
};

class VariableSymbol : public Symbol {
public:
    VariableSymbol(std::string name, const Type* type, SourceLocation loc)
        : Symbol(SymbolKind::Variable, std::move(name), type, std::move(loc)) {}
};

class ParameterSymbol : public Symbol {
public:
    ParameterSymbol(std::string name, const Type* type, SourceLocation loc)
        : Symbol(SymbolKind::Parameter, std::move(name), type, std::move(loc)) {}
};

class FunctionSymbol : public Symbol {
public:
    std::vector<const Type*> parameterTypes;
    std::vector<std::string> parameterNames;

    FunctionSymbol(std::string name, const Type* returnType,
                   std::vector<const Type*> paramTypes,
                   std::vector<std::string> paramNames,
                   SourceLocation loc)
        : Symbol(SymbolKind::Function, std::move(name), returnType,
                 std::move(loc)),
          parameterTypes(std::move(paramTypes)),
          parameterNames(std::move(paramNames)) {}

    const Type* returnType() const { return type_; }
};

} // namespace cc

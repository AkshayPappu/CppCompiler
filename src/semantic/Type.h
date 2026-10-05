#pragma once

#include <string>

namespace cc {

enum class TypeKind {
    Int,
    Bool,
    Void,
    Error
};

class Type {
public:
    explicit Type(TypeKind kind) : kind_(kind) {}

    TypeKind kind() const { return kind_; }

    bool isInt()   const { return kind_ == TypeKind::Int; }
    bool isBool()  const { return kind_ == TypeKind::Bool; }
    bool isVoid()  const { return kind_ == TypeKind::Void; }
    bool isError() const { return kind_ == TypeKind::Error; }

    bool operator==(const Type& other) const { return kind_ == other.kind_; }
    bool operator!=(const Type& other) const { return kind_ != other.kind_; }

    const char* toString() const {
        switch (kind_) {
            case TypeKind::Int:   return "int";
            case TypeKind::Bool:  return "bool";
            case TypeKind::Void:  return "void";
            case TypeKind::Error: return "<error>";
        }
        return "<unknown>";
    }

private:
    TypeKind kind_;
};

// Singleton type instances for the three supported primitive types,
// plus an Error sentinel used for error recovery during analysis.
// Future complex types (pointers, arrays, structs) would be allocated
// dynamically and owned by a TypeContext.
namespace Types {
    inline const Type Int{TypeKind::Int};
    inline const Type Bool{TypeKind::Bool};
    inline const Type Void{TypeKind::Void};
    inline const Type Error{TypeKind::Error};

    inline const Type* fromString(const std::string& name) {
        if (name == "int")  return &Int;
        if (name == "bool") return &Bool;
        if (name == "void") return &Void;
        return nullptr;
    }
}

} // namespace cc

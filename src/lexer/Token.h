#pragma once

#include "common/SourceLocation.h"
#include <cstdint>
#include <string>

namespace cc {

enum class TokenKind {
    // Keywords
    KW_Int,
    KW_Bool,
    KW_Void,
    KW_If,
    KW_Else,
    KW_While,
    KW_Return,
    KW_True,
    KW_False,

    // Identifiers and literals
    Identifier,
    IntegerLiteral,

    // Arithmetic operators
    Plus,          // +
    Minus,         // -
    Star,          // *
    Slash,         // /
    Percent,       // %

    // Assignment
    Assign,        // =

    // Comparison operators
    EqualEqual,    // ==
    BangEqual,     // !=
    Less,          // <
    LessEqual,     // <=
    Greater,       // >
    GreaterEqual,  // >=

    // Logical operators
    Bang,          // !
    AmpAmp,        // &&
    PipePipe,      // ||

    // Punctuation
    LeftParen,     // (
    RightParen,    // )
    LeftBrace,     // {
    RightBrace,    // }
    Comma,         // ,
    Semicolon,     // ;

    // Special
    Eof,
    Error
};

inline const char* tokenKindToString(TokenKind kind) {
    switch (kind) {
        case TokenKind::KW_Int:         return "int";
        case TokenKind::KW_Bool:        return "bool";
        case TokenKind::KW_Void:        return "void";
        case TokenKind::KW_If:          return "if";
        case TokenKind::KW_Else:        return "else";
        case TokenKind::KW_While:       return "while";
        case TokenKind::KW_Return:      return "return";
        case TokenKind::KW_True:        return "true";
        case TokenKind::KW_False:       return "false";
        case TokenKind::Identifier:     return "identifier";
        case TokenKind::IntegerLiteral: return "integer_literal";
        case TokenKind::Plus:           return "+";
        case TokenKind::Minus:          return "-";
        case TokenKind::Star:           return "*";
        case TokenKind::Slash:          return "/";
        case TokenKind::Percent:        return "%";
        case TokenKind::Assign:         return "=";
        case TokenKind::EqualEqual:     return "==";
        case TokenKind::BangEqual:      return "!=";
        case TokenKind::Less:           return "<";
        case TokenKind::LessEqual:      return "<=";
        case TokenKind::Greater:        return ">";
        case TokenKind::GreaterEqual:   return ">=";
        case TokenKind::Bang:           return "!";
        case TokenKind::AmpAmp:         return "&&";
        case TokenKind::PipePipe:       return "||";
        case TokenKind::LeftParen:      return "(";
        case TokenKind::RightParen:     return ")";
        case TokenKind::LeftBrace:      return "{";
        case TokenKind::RightBrace:     return "}";
        case TokenKind::Comma:          return ",";
        case TokenKind::Semicolon:      return ";";
        case TokenKind::Eof:            return "eof";
        case TokenKind::Error:          return "error";
    }
    return "unknown";
}

struct Token {
    TokenKind kind;
    std::string text;
    SourceLocation location;

    int64_t intValue = 0;  // populated for IntegerLiteral
    bool boolValue = false; // populated for KW_True / KW_False

    bool is(TokenKind k) const { return kind == k; }
    bool isNot(TokenKind k) const { return kind != k; }
    bool isOneOf(TokenKind k1, TokenKind k2) const { return is(k1) || is(k2); }

    template <typename... Ts>
    bool isOneOf(TokenKind k, Ts... ks) const {
        return is(k) || isOneOf(ks...);
    }
};

} // namespace cc

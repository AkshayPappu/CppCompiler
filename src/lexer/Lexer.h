#pragma once

#include "common/Diagnostic.h"
#include "lexer/Token.h"
#include <string>
#include <vector>

namespace cc {

class Lexer {
public:
    Lexer(const std::string& source, const std::string& filename,
          DiagnosticEngine& diag);

    Token nextToken();
    std::vector<Token> tokenizeAll();

private:
    char peek() const;
    char peekNext() const;
    char advance();
    bool isAtEnd() const;
    bool match(char expected);

    void skipWhitespace();
    void skipLineComment();
    bool skipBlockComment();

    Token makeToken(TokenKind kind) const;
    Token makeToken(TokenKind kind, const std::string& text) const;
    Token errorToken(const std::string& message);

    Token scanToken();
    Token scanIdentifierOrKeyword();
    Token scanNumber();

    TokenKind identifierKind(const std::string& text) const;

    std::string source_;
    std::string filename_;
    DiagnosticEngine& diag_;

    size_t start_   = 0;
    size_t current_ = 0;
    int line_       = 1;
    int column_     = 1;
    int startLine_  = 1;
    int startCol_   = 1;
};

} // namespace cc

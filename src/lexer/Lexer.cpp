#include "lexer/Lexer.h"
#include <cctype>

namespace cc {

Lexer::Lexer(const std::string& source, const std::string& filename,
             DiagnosticEngine& diag)
    : source_(source), filename_(filename), diag_(diag) {}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source_[current_];
}

char Lexer::peekNext() const {
    if (current_ + 1 >= source_.size()) return '\0';
    return source_[current_ + 1];
}

char Lexer::advance() {
    char c = source_[current_++];
    if (c == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return c;
}

bool Lexer::isAtEnd() const {
    return current_ >= source_.size();
}

bool Lexer::match(char expected) {
    if (isAtEnd() || source_[current_] != expected) return false;
    advance();
    return true;
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peekNext() == '/') {
            skipLineComment();
        } else if (c == '/' && peekNext() == '*') {
            if (!skipBlockComment()) return;
        } else {
            break;
        }
    }
}

void Lexer::skipLineComment() {
    advance(); // consume '/'
    advance(); // consume '/'
    while (!isAtEnd() && peek() != '\n') {
        advance();
    }
}

bool Lexer::skipBlockComment() {
    SourceLocation commentStart = {filename_, line_, column_};
    advance(); // consume '/'
    advance(); // consume '*'
    while (!isAtEnd()) {
        if (peek() == '*' && peekNext() == '/') {
            advance(); // consume '*'
            advance(); // consume '/'
            return true;
        }
        advance();
    }
    diag_.error(commentStart, "unterminated block comment");
    return false;
}

Token Lexer::makeToken(TokenKind kind) const {
    std::string text = source_.substr(start_, current_ - start_);
    return {kind, text, {filename_, startLine_, startCol_}};
}

Token Lexer::makeToken(TokenKind kind, const std::string& text) const {
    return {kind, text, {filename_, startLine_, startCol_}};
}

Token Lexer::errorToken(const std::string& message) {
    SourceLocation loc = {filename_, startLine_, startCol_};
    diag_.error(loc, message);
    return {TokenKind::Error, message, loc};
}

Token Lexer::nextToken() {
    skipWhitespace();

    start_    = current_;
    startLine_ = line_;
    startCol_  = column_;

    if (isAtEnd()) {
        return makeToken(TokenKind::Eof, "");
    }

    return scanToken();
}

Token Lexer::scanToken() {
    char c = advance();

    // Single-character tokens
    switch (c) {
        case '(': return makeToken(TokenKind::LeftParen);
        case ')': return makeToken(TokenKind::RightParen);
        case '{': return makeToken(TokenKind::LeftBrace);
        case '}': return makeToken(TokenKind::RightBrace);
        case ',': return makeToken(TokenKind::Comma);
        case ';': return makeToken(TokenKind::Semicolon);
        case '+': return makeToken(TokenKind::Plus);
        case '-': return makeToken(TokenKind::Minus);
        case '*': return makeToken(TokenKind::Star);
        case '/': return makeToken(TokenKind::Slash);
        case '%': return makeToken(TokenKind::Percent);
        default: break;
    }

    // Two-character operators
    if (c == '=') {
        return match('=') ? makeToken(TokenKind::EqualEqual)
                          : makeToken(TokenKind::Assign);
    }
    if (c == '!') {
        return match('=') ? makeToken(TokenKind::BangEqual)
                          : makeToken(TokenKind::Bang);
    }
    if (c == '<') {
        return match('=') ? makeToken(TokenKind::LessEqual)
                          : makeToken(TokenKind::Less);
    }
    if (c == '>') {
        return match('=') ? makeToken(TokenKind::GreaterEqual)
                          : makeToken(TokenKind::Greater);
    }
    if (c == '&') {
        if (match('&')) return makeToken(TokenKind::AmpAmp);
        return errorToken("unexpected character '&'; did you mean '&&'?");
    }
    if (c == '|') {
        if (match('|')) return makeToken(TokenKind::PipePipe);
        return errorToken("unexpected character '|'; did you mean '||'?");
    }

    // Numbers
    if (std::isdigit(c)) {
        return scanNumber();
    }

    // Identifiers and keywords
    if (std::isalpha(c) || c == '_') {
        return scanIdentifierOrKeyword();
    }

    std::string msg = "unexpected character '";
    msg += c;
    msg += "'";
    return errorToken(msg);
}

Token Lexer::scanNumber() {
    while (!isAtEnd() && std::isdigit(peek())) {
        advance();
    }

    if (!isAtEnd() && (std::isalpha(peek()) || peek() == '_')) {
        while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_')) {
            advance();
        }
        return errorToken("invalid integer literal suffix");
    }

    Token tok = makeToken(TokenKind::IntegerLiteral);
    tok.intValue = std::stoll(tok.text);
    return tok;
}

Token Lexer::scanIdentifierOrKeyword() {
    while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_')) {
        advance();
    }

    std::string text = source_.substr(start_, current_ - start_);
    TokenKind kind = identifierKind(text);

    Token tok = makeToken(kind, text);
    if (kind == TokenKind::KW_True) tok.boolValue = true;
    if (kind == TokenKind::KW_False) tok.boolValue = false;
    return tok;
}

TokenKind Lexer::identifierKind(const std::string& text) const {
    if (text == "int")    return TokenKind::KW_Int;
    if (text == "bool")   return TokenKind::KW_Bool;
    if (text == "void")   return TokenKind::KW_Void;
    if (text == "if")     return TokenKind::KW_If;
    if (text == "else")   return TokenKind::KW_Else;
    if (text == "while")  return TokenKind::KW_While;
    if (text == "return") return TokenKind::KW_Return;
    if (text == "true")   return TokenKind::KW_True;
    if (text == "false")  return TokenKind::KW_False;
    return TokenKind::Identifier;
}

std::vector<Token> Lexer::tokenizeAll() {
    std::vector<Token> tokens;
    while (true) {
        Token tok = nextToken();
        tokens.push_back(tok);
        if (tok.is(TokenKind::Eof) || tok.is(TokenKind::Error)) break;
    }
    return tokens;
}

} // namespace cc

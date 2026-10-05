#include "parser/Parser.h"

namespace cc {

Parser::Parser(const std::vector<Token>& tokens, DiagnosticEngine& diag)
    : tokens_(tokens), diag_(diag) {}

// ============================================================================
// Token stream navigation
// ============================================================================

const Token& Parser::peek() const {
    return tokens_[current_];
}

const Token& Parser::previous() const {
    return tokens_[current_ - 1];
}

const Token& Parser::advance() {
    if (!isAtEnd()) ++current_;
    return previous();
}

bool Parser::isAtEnd() const {
    return peek().is(TokenKind::Eof);
}

bool Parser::check(TokenKind kind) const {
    return peek().is(kind);
}

bool Parser::match(TokenKind kind) {
    if (check(kind)) {
        advance();
        return true;
    }
    return false;
}

const Token& Parser::expect(TokenKind kind, const std::string& message) {
    if (check(kind)) return advance();
    diag_.error(peek().location, message);
    // Return the current token to avoid crashing; callers must check diag
    return peek();
}

bool Parser::isTypeName() const {
    return peek().isOneOf(TokenKind::KW_Int, TokenKind::KW_Bool,
                          TokenKind::KW_Void);
}

// ============================================================================
// Error recovery: skip tokens until we find a likely statement boundary
// ============================================================================

void Parser::synchronize() {
    while (!isAtEnd()) {
        if (previous().is(TokenKind::Semicolon)) return;

        switch (peek().kind) {
            case TokenKind::KW_Int:
            case TokenKind::KW_Bool:
            case TokenKind::KW_Void:
            case TokenKind::KW_If:
            case TokenKind::KW_While:
            case TokenKind::KW_Return:
            case TokenKind::RightBrace:
                return;
            default:
                advance();
        }
    }
}

// ============================================================================
// Top-level parsing
// ============================================================================

std::unique_ptr<TranslationUnit> Parser::parseTranslationUnit() {
    auto unit = std::make_unique<TranslationUnit>();

    while (!isAtEnd()) {
        auto fn = parseFunctionDecl();
        if (fn) {
            unit->functions.push_back(std::move(fn));
        } else {
            synchronize();
            if (!isAtEnd()) advance();
        }
    }

    return unit;
}

// ============================================================================
// Declarations
// ============================================================================

std::unique_ptr<FunctionDecl> Parser::parseFunctionDecl() {
    if (!isTypeName()) {
        diag_.error(peek().location,
                    "expected type name at start of function declaration");
        return nullptr;
    }

    SourceLocation loc = peek().location;
    std::string returnType = advance().text;

    const Token& nameTok = expect(TokenKind::Identifier,
                                   "expected function name");
    std::string name = nameTok.text;

    expect(TokenKind::LeftParen, "expected '(' after function name");

    auto params = parseParameterList();

    expect(TokenKind::RightParen, "expected ')' after parameter list");

    auto body = parseBlock();
    if (!body) return nullptr;

    return std::make_unique<FunctionDecl>(
        loc, std::move(returnType), std::move(name),
        std::move(params), std::move(body));
}

std::vector<Parameter> Parser::parseParameterList() {
    std::vector<Parameter> params;

    if (check(TokenKind::RightParen)) return params;

    do {
        if (!isTypeName()) {
            diag_.error(peek().location, "expected parameter type");
            break;
        }

        SourceLocation loc = peek().location;
        std::string type = advance().text;

        const Token& nameTok = expect(TokenKind::Identifier,
                                       "expected parameter name");
        params.push_back({std::move(type), nameTok.text, loc});
    } while (match(TokenKind::Comma));

    return params;
}

// ============================================================================
// Statements
// ============================================================================

std::unique_ptr<Stmt> Parser::parseStatement() {
    if (check(TokenKind::LeftBrace))  return parseBlock();
    if (check(TokenKind::KW_If))      return parseIfStatement();
    if (check(TokenKind::KW_While))   return parseWhileStatement();
    if (check(TokenKind::KW_Return))  return parseReturnStatement();

    if (peek().isOneOf(TokenKind::KW_Int, TokenKind::KW_Bool)) {
        return parseVarDeclStatement();
    }

    return parseExpressionStatement();
}

std::unique_ptr<BlockStmt> Parser::parseBlock() {
    SourceLocation loc = peek().location;
    expect(TokenKind::LeftBrace, "expected '{'");

    std::vector<std::unique_ptr<Stmt>> stmts;

    while (!check(TokenKind::RightBrace) && !isAtEnd()) {
        auto stmt = parseStatement();
        if (stmt) {
            stmts.push_back(std::move(stmt));
        } else {
            synchronize();
        }
    }

    expect(TokenKind::RightBrace, "expected '}'");

    return std::make_unique<BlockStmt>(loc, std::move(stmts));
}

std::unique_ptr<Stmt> Parser::parseIfStatement() {
    SourceLocation loc = peek().location;
    advance(); // consume 'if'

    expect(TokenKind::LeftParen, "expected '(' after 'if'");
    auto condition = parseExpression();
    expect(TokenKind::RightParen, "expected ')' after if condition");

    auto thenBranch = parseBlock();

    std::unique_ptr<Stmt> elseBranch = nullptr;
    if (match(TokenKind::KW_Else)) {
        if (check(TokenKind::KW_If)) {
            elseBranch = parseIfStatement();
        } else {
            elseBranch = parseBlock();
        }
    }

    return std::make_unique<IfStmt>(
        loc, std::move(condition), std::move(thenBranch),
        std::move(elseBranch));
}

std::unique_ptr<Stmt> Parser::parseWhileStatement() {
    SourceLocation loc = peek().location;
    advance(); // consume 'while'

    expect(TokenKind::LeftParen, "expected '(' after 'while'");
    auto condition = parseExpression();
    expect(TokenKind::RightParen, "expected ')' after while condition");

    auto body = parseBlock();

    return std::make_unique<WhileStmt>(loc, std::move(condition),
                                        std::move(body));
}

std::unique_ptr<Stmt> Parser::parseReturnStatement() {
    SourceLocation loc = peek().location;
    advance(); // consume 'return'

    std::unique_ptr<Expr> value = nullptr;
    if (!check(TokenKind::Semicolon)) {
        value = parseExpression();
    }

    expect(TokenKind::Semicolon, "expected ';' after return statement");

    return std::make_unique<ReturnStmt>(loc, std::move(value));
}

std::unique_ptr<Stmt> Parser::parseVarDeclStatement() {
    SourceLocation loc = peek().location;
    std::string typeName = advance().text;

    const Token& nameTok = expect(TokenKind::Identifier,
                                   "expected variable name");
    std::string varName = nameTok.text;

    std::unique_ptr<Expr> init = nullptr;
    if (match(TokenKind::Assign)) {
        init = parseExpression();
    }

    expect(TokenKind::Semicolon, "expected ';' after variable declaration");

    return std::make_unique<VarDeclStmt>(
        loc, std::move(typeName), std::move(varName), std::move(init));
}

std::unique_ptr<Stmt> Parser::parseExpressionStatement() {
    SourceLocation loc = peek().location;
    auto expr = parseExpression();

    expect(TokenKind::Semicolon, "expected ';' after expression");

    return std::make_unique<ExpressionStmt>(loc, std::move(expr));
}

// ============================================================================
// Expressions
// ============================================================================

std::unique_ptr<Expr> Parser::parseExpression() {
    return parseAssignment();
}

std::unique_ptr<Expr> Parser::parseAssignment() {
    auto expr = parseBinary(0);

    if (match(TokenKind::Assign)) {
        SourceLocation loc = previous().location;

        auto* ident = dynamic_cast<IdentifierExpr*>(expr.get());
        if (!ident) {
            diag_.error(loc, "invalid assignment target");
            return expr;
        }

        std::string name = ident->name;
        auto value = parseAssignment(); // right-associative
        return std::make_unique<AssignmentExpr>(
            loc, std::move(name), std::move(value));
    }

    return expr;
}

// Precedence levels for binary operators (higher = tighter binding)
int Parser::getBinaryPrecedence(TokenKind kind) const {
    switch (kind) {
        case TokenKind::PipePipe:                          return 1;
        case TokenKind::AmpAmp:                            return 2;
        case TokenKind::EqualEqual: case TokenKind::BangEqual:
            return 3;
        case TokenKind::Less: case TokenKind::LessEqual:
        case TokenKind::Greater: case TokenKind::GreaterEqual:
            return 4;
        case TokenKind::Plus: case TokenKind::Minus:       return 5;
        case TokenKind::Star: case TokenKind::Slash:
        case TokenKind::Percent:                           return 6;
        default:                                           return -1;
    }
}

BinaryOp Parser::tokenToBinaryOp(TokenKind kind) const {
    switch (kind) {
        case TokenKind::Plus:         return BinaryOp::Add;
        case TokenKind::Minus:        return BinaryOp::Sub;
        case TokenKind::Star:         return BinaryOp::Mul;
        case TokenKind::Slash:        return BinaryOp::Div;
        case TokenKind::Percent:      return BinaryOp::Mod;
        case TokenKind::EqualEqual:   return BinaryOp::Equal;
        case TokenKind::BangEqual:    return BinaryOp::NotEqual;
        case TokenKind::Less:         return BinaryOp::Less;
        case TokenKind::LessEqual:    return BinaryOp::LessEqual;
        case TokenKind::Greater:      return BinaryOp::Greater;
        case TokenKind::GreaterEqual: return BinaryOp::GreaterEqual;
        case TokenKind::AmpAmp:       return BinaryOp::LogicalAnd;
        case TokenKind::PipePipe:     return BinaryOp::LogicalOr;
        default:                      return BinaryOp::Add; // unreachable
    }
}

std::unique_ptr<Expr> Parser::parseBinary(int minPrecedence) {
    auto left = parseUnary();

    while (true) {
        int prec = getBinaryPrecedence(peek().kind);
        if (prec < minPrecedence) break;

        SourceLocation loc = peek().location;
        TokenKind opKind = advance().kind;
        BinaryOp op = tokenToBinaryOp(opKind);

        // All binary operators are left-associative, so use prec + 1
        auto right = parseBinary(prec + 1);

        left = std::make_unique<BinaryExpr>(
            loc, op, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<Expr> Parser::parseUnary() {
    if (check(TokenKind::Minus)) {
        SourceLocation loc = peek().location;
        advance();
        auto operand = parseUnary();
        return std::make_unique<UnaryExpr>(loc, UnaryOp::Negate,
                                            std::move(operand));
    }

    if (check(TokenKind::Bang)) {
        SourceLocation loc = peek().location;
        advance();
        auto operand = parseUnary();
        return std::make_unique<UnaryExpr>(loc, UnaryOp::LogicalNot,
                                            std::move(operand));
    }

    return parsePrimary();
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    // Integer literal
    if (check(TokenKind::IntegerLiteral)) {
        SourceLocation loc = peek().location;
        int64_t value = peek().intValue;
        advance();
        return std::make_unique<IntegerLiteralExpr>(loc, value);
    }

    // Boolean literal
    if (check(TokenKind::KW_True)) {
        SourceLocation loc = peek().location;
        advance();
        return std::make_unique<BooleanLiteralExpr>(loc, true);
    }
    if (check(TokenKind::KW_False)) {
        SourceLocation loc = peek().location;
        advance();
        return std::make_unique<BooleanLiteralExpr>(loc, false);
    }

    // Identifier or function call
    if (check(TokenKind::Identifier)) {
        SourceLocation loc = peek().location;
        std::string name = advance().text;

        if (check(TokenKind::LeftParen)) {
            return parseCallExpr(name, loc);
        }

        return std::make_unique<IdentifierExpr>(loc, std::move(name));
    }

    // Parenthesized expression
    if (match(TokenKind::LeftParen)) {
        auto expr = parseExpression();
        expect(TokenKind::RightParen, "expected ')' after expression");
        return expr;
    }

    diag_.error(peek().location,
                "expected expression, got '" + peek().text + "'");
    advance(); // skip the problematic token
    return std::make_unique<IntegerLiteralExpr>(peek().location, 0);
}

std::unique_ptr<Expr> Parser::parseCallExpr(const std::string& name,
                                              SourceLocation loc) {
    advance(); // consume '('

    std::vector<std::unique_ptr<Expr>> args;

    if (!check(TokenKind::RightParen)) {
        do {
            args.push_back(parseExpression());
        } while (match(TokenKind::Comma));
    }

    expect(TokenKind::RightParen, "expected ')' after function arguments");

    return std::make_unique<CallExpr>(
        loc, name, std::move(args));
}

} // namespace cc

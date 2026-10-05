# CppCompiler

A C++ compiler for a subset of C++, built entirely from scratch in C++17. No parser generators, no LLVM — just handwritten compiler infrastructure.

## Compiler Pipeline

Currently only the frontend is implemented. The full planned pipeline:

```
C++ Source → Lexer → Parser → AST → Semantic Analysis → IR → Optimization → x86-64
                     ^^^^^^^^^^^^^^^^^^^
                     implemented (v1)
```

## Architecture

### Lexer

The lexer (`src/lexer/`) is a single-pass character scanner that converts source text into a stream of tokens. It handles whitespace, single-line comments (`//`), and block comments (`/* */`). Each token carries its source location (filename, line, column) for diagnostics.

Invalid characters and unterminated block comments are caught and reported at this stage.

### Parser

The parser (`src/parser/`) is a **top-down recursive descent** parser — the same approach used by GCC, Clang, Rust, and Go. Each grammar rule maps to a function:

- `parseTranslationUnit()` parses the top-level list of function declarations
- `parseFunctionDecl()` parses return type, name, parameters, and body
- `parseStatement()` dispatches to `parseIfStatement()`, `parseWhileStatement()`, etc.
- `parseExpression()` uses **precedence climbing** for binary operators

Expression parsing handles operator precedence without needing parentheses in the grammar:

```
Assignment  =           (lowest, right-associative)
Logical OR  ||
Logical AND &&
Equality    == !=
Comparison  < <= > >=
Additive    + -
Multiply    * / %
Unary       - !
Primary     literals, identifiers, calls, (...)   (highest)
```

Syntax errors are reported with `file:line:column` locations. The parser attempts error recovery to report multiple errors in a single run.

### AST

The AST (`src/ast/`) is a class hierarchy using inheritance and `std::unique_ptr` for ownership:

```
ASTNode
├── TranslationUnit          (root: list of function declarations)
├── FunctionDecl             (return type, name, params, body)
├── Stmt
│   ├── BlockStmt            { ... }
│   ├── IfStmt               if/else
│   ├── WhileStmt            while loops
│   ├── ReturnStmt           return with optional value
│   ├── ExpressionStmt       expression used as statement
│   └── VarDeclStmt          variable declaration
└── Expr
    ├── BinaryExpr           a + b, x == y, etc.
    ├── UnaryExpr            -x, !flag
    ├── AssignmentExpr       x = 5
    ├── CallExpr             foo(a, b)
    ├── IdentifierExpr       variable reference
    ├── IntegerLiteralExpr   42
    └── BooleanLiteralExpr   true, false
```

The AST uses the **visitor pattern** (`ASTVisitor`), so future compiler passes (semantic analysis, IR generation) can traverse the tree without modifying node classes.

### Diagnostics

All errors carry source location information and are printed in standard compiler format:

```
tests/example.cpp:4:12: error: expected ')' after condition
```

## Supported C++ Features

| Feature | Status |
|---|---|
| Types: `int`, `bool`, `void` | Supported |
| Integer literals (`0`, `42`, `1234`) | Supported |
| Boolean literals (`true`, `false`) | Supported |
| Variable declarations (`int x = 5;`) | Supported |
| Assignment (`x = 10;`, `a = b = 5;`) | Supported |
| Arithmetic (`+`, `-`, `*`, `/`, `%`) | Supported |
| Comparison (`==`, `!=`, `<`, `<=`, `>`, `>=`) | Supported |
| Logical operators (`&&`, `\|\|`, `!`) | Supported |
| Unary operators (`-x`, `!flag`) | Supported |
| `if` / `else` | Supported |
| `while` loops | Supported |
| Functions (definitions, calls, return) | Supported |
| Multiple parameters | Supported |
| Nested blocks / scopes | Supported |
| Single-line comments (`//`) | Supported |
| Block comments (`/* */`) | Supported |
| Operator precedence | Supported |

## Not Yet Supported

These are intentionally excluded from version 1:

- `float`, `double`, `char`, `short`, `long`, `unsigned`, `auto`
- `for` loops, `do-while`, `switch`, `break`, `continue`
- `++`, `--` (increment/decrement)
- Bitwise operators (`&`, `|`, `^`, `~`, `<<`, `>>`)
- Pointers, references, arrays
- Strings, character literals
- Classes, structs, unions, enums
- Templates, namespaces, lambdas
- Operator overloading, function overloading
- Default arguments, variadic functions
- `new`, `delete`, exceptions (`try`/`catch`)
- Preprocessor (`#include`, `#define`)
- `typedef`, `using`, `constexpr`, `static`, `extern`
- Inheritance, virtual functions, access modifiers
- STL, standard library
- Semantic analysis (type checking, scope resolution)
- IR generation, optimization, code generation

## Building

```bash
cmake -G "MinGW Makefiles" -B build -S .
cmake --build build
```

## Usage

```bash
# Parse a file and print the AST (default)
.\build\cppc.exe tests\example.cpp

# Print the token stream from the lexer
.\build\cppc.exe --dump-tokens tests\example.cpp

# Show help
.\build\cppc.exe --help
```

## Project Structure

```
src/
├── main.cpp              # CLI entry point
├── common/
│   ├── SourceLocation.h  # file:line:column tracking
│   └── Diagnostic.h      # error/warning reporting engine
├── lexer/
│   ├── Token.h           # token types and data
│   ├── Lexer.h           # lexer interface
│   └── Lexer.cpp         # character-by-character scanner
├── ast/
│   ├── AST.h             # AST node class hierarchy
│   └── ASTPrinter.h      # indented tree printer (visitor)
└── parser/
    ├── Parser.h           # parser interface
    └── Parser.cpp         # recursive descent + precedence climbing

tests/
├── example.cpp           # full-featured test program
├── precedence.cpp        # operator precedence edge cases
└── errors.cpp            # intentionally broken code for error testing
```

## Example

Given this input:

```cpp
int max(int a, int b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}
```

The compiler produces this AST:

```
TranslationUnit
  FunctionDecl: int max(int a, int b)
    BlockStmt
      IfStmt
        Condition:
          BinaryExpr: >
            IdentifierExpr: a
            IdentifierExpr: b
        Then:
          BlockStmt
            ReturnStmt
              IdentifierExpr: a
        Else:
          BlockStmt
            ReturnStmt
              IdentifierExpr: b
```

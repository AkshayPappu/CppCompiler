# CppCompiler

A compiler for a small C++ subset, written from scratch in C++17. No parser generators.

```
Source → Lexer → Parser → AST → Semantic Analysis → LLVM IR
```

Native object files and executables are not implemented yet.

## Language subset

**Supported:** `int`, `bool`, `void`; locals and assignment; `+ - * / %`; comparisons; `&& || !`; unary `-`; `if`/`else`; `while`; functions and `return`; `//` and `/* */` comments.

**Not supported:** other types, `for`/`switch`/`break`, `++`/`--`, pointers, arrays, classes, templates, the preprocessor, STL, and most of the rest of C++.

There are no implicit conversions (e.g. `if (5)` is an error). `&&` and `||` short-circuit.

## Build

Requires C++17, CMake, and LLVM (tested with 22.1.8 from MSYS2).

```bash
cmake -B build -S .
cmake --build build
```

On Windows with MSYS2, put `C:\msys64\ucrt64\bin` on `PATH`, or build so the MinGW runtime DLLs sit next to `cppc.exe`.

## Usage

```bash
.\build\cppc.exe tests\example.cpp              # type-check + generate IR
.\build\cppc.exe --emit-llvm tests\example.cpp  # print LLVM IR
.\build\cppc.exe --dump-tokens tests\example.cpp
.\build\cppc.exe --dump-ast tests\example.cpp
.\build\cppc.exe --dump-sema tests\example.cpp
.\build\cppc.exe --emit-llvm tests\example.cpp -o out.ll   # optional file
```

## Layout

```
src/common/     source locations, diagnostics
src/lexer/      tokenizer
src/parser/     recursive descent + precedence climbing
src/ast/        AST + printer
src/semantic/   types, symbols, scopes, type checker
src/codegen/    LLVM IR generator
tests/          example programs
tests/sema/     semantic-analysis cases
tests/codegen/  IR-generation cases
```

## Next

LLVM optimization and x86-64 object/executable generation.

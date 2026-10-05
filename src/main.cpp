#include "ast/ASTPrinter.h"
#include "codegen/LLVMCodeGen.h"
#include "common/Diagnostic.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "semantic/SemaPrinter.h"
#include "semantic/SemanticAnalyzer.h"

#include <llvm/Support/raw_ostream.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "error: could not open file '" << path << "'\n";
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static void printUsage(const char* program) {
    std::cerr << "Usage: " << program << " [options] <source-file>\n"
              << "\nOptions:\n"
              << "  --dump-tokens   Print the token stream and exit\n"
              << "  --dump-ast      Parse and print the AST\n"
              << "  --dump-sema     Run semantic analysis and print annotated AST\n"
              << "  --emit-llvm     Generate LLVM IR and print it to stdout\n"
              << "  -o <file>       With --emit-llvm, write IR to this file instead\n"
              << "  --help          Show this help message\n"
              << "\nWith no options, runs the full frontend pipeline.\n";
}

int main(int argc, char* argv[]) {
    enum class Mode { Compile, DumpTokens, DumpAST, DumpSema, EmitLLVM };
    Mode mode = Mode::Compile;
    std::string filename;
    std::string outputPath;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--dump-tokens") {
            mode = Mode::DumpTokens;
        } else if (arg == "--dump-ast") {
            mode = Mode::DumpAST;
        } else if (arg == "--dump-sema") {
            mode = Mode::DumpSema;
        } else if (arg == "--emit-llvm") {
            mode = Mode::EmitLLVM;
        } else if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "error: -o requires a filename\n";
                return 1;
            }
            outputPath = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg[0] == '-') {
            std::cerr << "error: unknown option '" << arg << "'\n";
            printUsage(argv[0]);
            return 1;
        } else {
            filename = arg;
        }
    }

    if (filename.empty()) {
        std::cerr << "error: no input file\n";
        printUsage(argv[0]);
        return 1;
    }

    std::string source = readFile(filename);
    if (source.empty() && std::ifstream(filename).peek() == std::ifstream::traits_type::eof()) {
        // File exists but is empty — allow it
    } else if (source.empty()) {
        return 1;
    }

    cc::DiagnosticEngine diag;

    // --dump-tokens: lex only
    if (mode == Mode::DumpTokens) {
        cc::Lexer lexer(source, filename, diag);
        while (true) {
            cc::Token tok = lexer.nextToken();
            std::cout << tok.location.toString()
                      << ": " << cc::tokenKindToString(tok.kind);
            if (tok.is(cc::TokenKind::Identifier) ||
                tok.is(cc::TokenKind::IntegerLiteral)) {
                std::cout << " '" << tok.text << "'";
            }
            std::cout << "\n";
            if (tok.is(cc::TokenKind::Eof)) break;
        }
        return diag.hasErrors() ? 1 : 0;
    }

    // Lex
    cc::Lexer lexer(source, filename, diag);
    auto tokens = lexer.tokenizeAll();

    if (diag.hasErrors()) {
        std::cerr << "\n" << diag.errorCount() << " error(s) during lexing.\n";
        return 1;
    }

    // Parse
    cc::Parser parser(tokens, diag);
    auto ast = parser.parseTranslationUnit();

    if (diag.hasErrors()) {
        std::cerr << "\n" << diag.errorCount() << " error(s) during parsing.\n";
        return 1;
    }

    // --dump-ast: stop after parsing
    if (mode == Mode::DumpAST) {
        std::cout << "=== AST ===\n";
        cc::ASTPrinter printer;
        printer.print(*ast);
        return 0;
    }

    // Semantic analysis
    cc::SemanticAnalyzer sema(diag);
    bool semaOk = sema.analyze(*ast);

    if (!semaOk) {
        std::cerr << "\n" << diag.errorCount()
                  << " error(s) during semantic analysis.\n";
        return 1;
    }

    // --dump-sema: print the semantically annotated AST
    if (mode == Mode::DumpSema) {
        cc::SemaPrinter printer;
        printer.print(*ast);
        return 0;
    }

    // LLVM IR generation (for both Compile and EmitLLVM modes)
    cc::LLVMCodeGen codegen;
    if (!codegen.generate(*ast)) {
        std::cerr << "\nLLVM IR generation failed.\n";
        return 1;
    }

    if (mode == Mode::EmitLLVM) {
        std::string ir;
        llvm::raw_string_ostream os(ir);
        codegen.printIR(os);
        os.flush();

        if (!outputPath.empty()) {
            std::ofstream outFile(outputPath);
            if (!outFile) {
                std::cerr << "error: could not write '" << outputPath << "'\n";
                return 1;
            }
            outFile << ir;
            return 0;
        }

        std::fwrite(ir.data(), 1, ir.size(), stdout);
        std::fflush(stdout);
        return 0;
    }

    // Default Compile mode
    std::cout << "Compilation successful.\n";
    return 0;
}

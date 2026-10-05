#include "ast/ASTPrinter.h"
#include "common/Diagnostic.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"

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
              << "  --dump-ast      Parse and print the AST (default)\n"
              << "  --help          Show this help message\n";
}

int main(int argc, char* argv[]) {
    enum class Mode { DumpTokens, DumpAST };
    Mode mode = Mode::DumpAST;
    std::string filename;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--dump-tokens") {
            mode = Mode::DumpTokens;
        } else if (arg == "--dump-ast") {
            mode = Mode::DumpAST;
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

    // Mode::DumpAST (default)
    cc::Lexer lexer(source, filename, diag);
    auto tokens = lexer.tokenizeAll();

    if (diag.hasErrors()) {
        std::cerr << "\n" << diag.errorCount() << " error(s) during lexing.\n";
        return 1;
    }

    cc::Parser parser(tokens, diag);
    auto ast = parser.parseTranslationUnit();

    if (diag.hasErrors()) {
        std::cerr << "\n" << diag.errorCount() << " error(s) during parsing.\n";
        return 1;
    }

    std::cout << "=== AST ===\n";
    cc::ASTPrinter printer;
    printer.print(*ast);

    return 0;
}

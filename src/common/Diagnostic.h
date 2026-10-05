#pragma once

#include "common/SourceLocation.h"
#include <iostream>
#include <string>
#include <vector>

namespace cc {

struct Diagnostic {
    enum class Level { Error, Warning };

    Level level;
    SourceLocation location;
    std::string message;

    std::string toString() const {
        std::string prefix = (level == Level::Error) ? "error" : "warning";
        return location.toString() + ": " + prefix + ": " + message;
    }
};

class DiagnosticEngine {
public:
    void report(Diagnostic::Level level, const SourceLocation& loc,
                const std::string& message) {
        diagnostics_.push_back({level, loc, message});
        if (level == Diagnostic::Level::Error) {
            ++errorCount_;
        }
        std::cerr << diagnostics_.back().toString() << "\n";
    }

    void error(const SourceLocation& loc, const std::string& message) {
        report(Diagnostic::Level::Error, loc, message);
    }

    void warning(const SourceLocation& loc, const std::string& message) {
        report(Diagnostic::Level::Warning, loc, message);
    }

    bool hasErrors() const { return errorCount_ > 0; }
    int errorCount() const { return errorCount_; }

    const std::vector<Diagnostic>& diagnostics() const { return diagnostics_; }

private:
    std::vector<Diagnostic> diagnostics_;
    int errorCount_ = 0;
};

} // namespace cc

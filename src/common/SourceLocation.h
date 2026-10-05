#pragma once

#include <string>

namespace cc {

struct SourceLocation {
    std::string filename;
    int line   = 1;
    int column = 1;

    std::string toString() const {
        return filename + ":" + std::to_string(line) + ":" + std::to_string(column);
    }
};

} // namespace cc

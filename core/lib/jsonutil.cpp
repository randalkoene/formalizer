// Copyright 2025 Randal A. Koene
// License TBD

/**
 * Utility functions associated with JSON I/O work.
 */

// std
#include <string>
#include <sstream>
#include <iomanip>

// core

namespace fz {

// The following function produces the shortest JSON representation of a UTF-8 encoded string s.
// (Based on https://stackoverflow.com/a/33799784 .)
std::string escape_for_json(const std::string& s) {
    std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        switch (*c) {
        case '"': o << "\\\""; break;
        case '\\': o << "\\\\"; break;
        case '\b': o << "\\b"; break;
        case '\f': o << "\\f"; break;
        case '\n': o << "\\n"; break;
        case '\r': o << "\\r"; break;
        case '\t': o << "\\t"; break;
        default:
            if ('\x00' <= *c && *c <= '\x1f') {
                o << "\\u"
                  << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(*c);
            } else {
                o << *c;
            }
        }
    }
    return o.str();
}

} // namespace fz

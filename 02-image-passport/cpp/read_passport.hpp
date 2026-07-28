#pragma once

#include <expected>
#include <iosfwd>
#include <string>

enum class PassportErrorKind { kNoInput, kEmptyName, kBadCount, kNegativeCount, kIOError };

struct PassportError {
    PassportErrorKind kind;
    std::string bad_value;
};

struct PassportData {
    std::string name;
    int count;
};

std::expected<PassportData, PassportError> read_passport(std::istream& is);

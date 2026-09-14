#ifndef KV_READ_PASSPORT_HPP
#define KV_READ_PASSPORT_HPP

#include <cstdint>
#include <expected>
#include <iosfwd>
#include <string>
#include <system_error>

namespace raster::passport {

enum class PassportErrorKind { kNoInput, kEmptyName, kBadCount, kNegativeCount, kIOError };

struct PassportError {
    PassportErrorKind kind;
    std::string bad_value;
    std::error_code system_error{};
};

struct PassportData {
    std::string name;
    int32_t count;
};

std::expected<PassportData, PassportError> read_passport(std::istream& is);

std::string passport_error_message(const PassportError& error);

} // namespace raster::passport

#endif

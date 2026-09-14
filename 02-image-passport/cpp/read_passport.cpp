#include "read_passport.hpp"

#include <cerrno>
#include <charconv>
#include <format>
#include <istream>
#include <print>
#include <string>
#include <system_error>

namespace raster::passport {

std::expected<PassportData, PassportError> read_passport(std::istream& is) {
    std::println("Введите название изображения: ");

    std::string name;
    std::getline(is, name);

    if (is.eof() && name.empty()) [[unlikely]]
        return std::unexpected(PassportError{PassportErrorKind::kNoInput, {}});

    if (is.fail() && !is.eof()) [[unlikely]]
        return std::unexpected(PassportError{
            PassportErrorKind::kIOError, {}, std::error_code(errno, std::generic_category())});

    while (!name.empty() && (name.back() == ' ' || name.back() == '\t'))
        name.pop_back();
    if (!name.empty()) {
        size_t start = name.find_first_not_of(" \t");
        if (start == std::string::npos)
            name.clear();
        else if (start != 0)
            name = name.substr(start);
    }

    if (name.empty()) [[unlikely]]
        return std::unexpected(PassportError{PassportErrorKind::kEmptyName, {}});

    std::println("Введите количество пикселей: ");

    std::string count_str;
    std::getline(is, count_str);

    if (is.eof() && count_str.empty()) [[unlikely]]
        return std::unexpected(PassportError{PassportErrorKind::kNoInput, {}});

    if (is.fail() && !is.eof()) [[unlikely]]
        return std::unexpected(PassportError{
            PassportErrorKind::kIOError, {}, std::error_code(errno, std::generic_category())});

    int32_t count{};
    auto [ptr, ec] = std::from_chars(count_str.data(), count_str.data() + count_str.size(), count);
    if (ec != std::errc{} || ptr != count_str.data() + count_str.size()) [[unlikely]]
        return std::unexpected(PassportError{PassportErrorKind::kBadCount, count_str});

    if (count <= 0) [[unlikely]]
        return std::unexpected(PassportError{PassportErrorKind::kNegativeCount, count_str});

    return PassportData{std::move(name), count};
}

std::string passport_error_message(const PassportError& error) {
    switch (error.kind) {
    case PassportErrorKind::kNoInput:
        return "Нет ввода";
    case PassportErrorKind::kEmptyName:
        return "Название изображения не может быть пустым";
    case PassportErrorKind::kBadCount:
        return std::format("количество пикселей должно быть числом; получено: {}", error.bad_value);
    case PassportErrorKind::kNegativeCount:
        return std::format("количество пикселей должно быть положительным; получено: {}",
                           error.bad_value);
    case PassportErrorKind::kIOError:
        return std::format("Сбой ввода: {} (errno {})", error.system_error.message(),
                           error.system_error.value());
    }
    return {};
}

} // namespace raster::passport
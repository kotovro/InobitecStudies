#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <print>
#include <string_view>
#include <system_error>
#include <utility>

#include "../../common/cpp/exit_codes.hpp"
#include "../../common/cpp/version.hpp"
#include "pixel_word.hpp"
#include "read_passport.hpp"

using namespace raster::common;
using namespace raster::passport;

namespace {

void print_usage() {
    std::println("Использование: read_passport");
    std::println();
    std::println("Интерактивно читает из stdin название изображения и число");
    std::println("пикселей, выводит фразу со склонением «пиксель/пикселя/пикселей».");
    std::println();
    std::println("Опции:");
    std::println("  --help      показать справку");
    std::println("  --version   показать версию");
}

void print_version() { std::println("read_passport {}", kVersion); }

} // namespace

int main(int argc, char** argv) {
    if (argc >= 2 && std::string_view(argv[1]) == "--help") {
        print_usage();
        return std::to_underlying(ExitCode::kOk);
    }

    if (argc >= 2 && std::string_view(argv[1]) == "--version") {
        print_version();
        return std::to_underlying(ExitCode::kOk);
    }

    auto result = read_passport(std::cin);
    if (!result) [[unlikely]] {
        switch (result.error().kind) {
        case PassportErrorKind::kNoInput:
            std::println(stderr, "Нет ввода");
            return std::to_underlying(ExitCode::kNoInput);
        case PassportErrorKind::kEmptyName:
            std::println(stderr, "Название изображения не может быть пустым");
            return std::to_underlying(ExitCode::kData);
        case PassportErrorKind::kBadCount:
            std::println(stderr, "количество пикселей должно быть числом; получено: {}",
                         result.error().bad_value);
            return std::to_underlying(ExitCode::kData);
        case PassportErrorKind::kNegativeCount:
            std::println(stderr, "количество пикселей должно быть положительным; получено: {}",
                         result.error().bad_value);
            return std::to_underlying(ExitCode::kData);
        case PassportErrorKind::kIOError:
            std::println(stderr, "Сбой ввода: {} (errno {})",
                         std::generic_category().message(errno), errno);
            return std::to_underlying(ExitCode::kIOErr);
        }
    }

    std::println("Изображение \xAB{}\xBB: {} {}.", result->name, result->count,
                 pixel_word(result->count));
    return std::to_underlying(ExitCode::kOk);
}
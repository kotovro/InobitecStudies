#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <locale>
#include <print>
#include <system_error>

#include "../../common/exit_codes.hpp"
#include "pixel_word.hpp"
#include "read_passport.hpp"

int main() {
    std::setlocale(LC_ALL, "Russian_Russia.1251");

    auto result = read_passport(std::cin);
    if (!result) [[unlikely]] {
        switch (result.error().kind) {
        case PassportErrorKind::kNoInput:
            std::println(stderr, "Нет ввода");
            return (int)ExitCode::kNoInput;
        case PassportErrorKind::kEmptyName:
            std::println(stderr, "Название изображения не может быть пустым");
            return (int)ExitCode::kData;
        case PassportErrorKind::kBadCount:
            std::println(stderr, "количество пикселей должно быть числом; получено: {}",
                         result.error().bad_value);
            return (int)ExitCode::kData;
        case PassportErrorKind::kNegativeCount:
            std::println(stderr, "количество пикселей должно быть положительным; получено: {}",
                         result.error().bad_value);
            return (int)ExitCode::kData;
        case PassportErrorKind::kIOError:
            std::println(stderr, "Сбой ввода: {} (errno {})",
                         std::generic_category().message(errno), errno);
            return (int)ExitCode::kIOErr;
        }
    }

    std::println("Изображение \xAB{}\xBB: {} {}.", result->name, result->count,
                 pixel_word(result->count));
    return (int)ExitCode::kOk;
}

#include "pixel_word.h"
#include "read_passport.h"

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <locale>
#include <print>
#include <system_error>

int main() {
    std::setlocale(LC_ALL, "Russian_Russia.1251");

    auto result = read_passport(std::cin);
    if (!result) [[unlikely]] {
        switch (result.error().kind) {
        case PassportErrorKind::kNoInput:
            std::println(stderr, "нет входных данных");
            return 66;
        case PassportErrorKind::kEmptyName:
            std::println(stderr, "название не может быть пустым");
            return 65;
        case PassportErrorKind::kBadCount:
            std::println(stderr, "количество пикселей должно быть целым числом; получено: {}",
                         result.error().bad_value);
            return 65;
        case PassportErrorKind::kNegativeCount:
            std::println(stderr, "количество пикселей должно быть положительным; получено: {}",
                         result.error().bad_value);
            return 65;
        case PassportErrorKind::kIOError:
            std::println(stderr, "сбой ввода: {} (errno {})",
                         std::generic_category().message(errno), errno);
            return 74;
        }
    }

    std::println("Изображение «{}»: {} {}.", result->name, result->count,
                 pixel_word(result->count));
    return 0;
}

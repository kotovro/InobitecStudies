#include "pixel_word.h"
#include "read_passport.h"

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <locale>
#include <print>
#include <system_error>

#include "../../common/exit_codes.hpp"

int main() {
    std::setlocale(LC_ALL, "Russian_Russia.1251");

    auto result = read_passport(std::cin);
    if (!result) [[unlikely]] {
        switch (result.error().kind) {
        case PassportErrorKind::kNoInput:
            std::println(stderr, "��� ������� ������");
            return (int)ExitCode::kNoInput;
        case PassportErrorKind::kEmptyName:
            std::println(stderr, "�������� �� ����� ���� ������");
            return (int)ExitCode::kData;
        case PassportErrorKind::kBadCount:
            std::println(stderr, "���������� �������� ������ ���� ����� ������; ��������: {}",
                         result.error().bad_value);
            return (int)ExitCode::kData;
        case PassportErrorKind::kNegativeCount:
            std::println(stderr, "���������� �������� ������ ���� �������������; ��������: {}",
                         result.error().bad_value);
            return (int)ExitCode::kData;
        case PassportErrorKind::kIOError:
            std::println(stderr, "���� �����: {} (errno {})",
                         std::generic_category().message(errno), errno);
            return (int)ExitCode::kIOErr;
        }
    }

    std::println("����������� �{}�: {} {}.", result->name, result->count,
                 pixel_word(result->count));
    return (int)ExitCode::kOk;
}
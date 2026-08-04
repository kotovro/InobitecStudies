#include "ppm_stats.hpp"

#include "../../common/cpp/version.hpp"

#include <cstdlib>
#include <iostream>
#include <locale>
#include <print>
#include <string_view>

void print_usage() {
    std::println("Использование: image_stats");
    std::println();
    std::println("Читает PPM P3 из stdin до EOF, выводит статистику:");
    std::println("  размеры, число пикселей, средний цвет, яркость, гистограмма");
    std::println();
    std::println("Работает в конвейере: gen_image | image_stats");
    std::println();
    std::println("Опции:");
    std::println("  --help      показать справку");
    std::println("  --version   показать версию");
}

void print_version() { std::println("image_stats {}", kVersion); }

int main(int argc, char** argv) {
    std::setlocale(LC_ALL, "Russian_Russia.1251");

    if (argc >= 2 && std::string_view(argv[1]) == "--help") {
        print_usage();
        return (int)ExitCode::kOk;
    }

    if (argc >= 2 && std::string_view(argv[1]) == "--version") {
        print_version();
        return (int)ExitCode::kOk;
    }

    auto result = ppm_read_stats(std::cin);
    if (!result.value) {
        switch (result.value.error()) {
        case StatsError::kEmptyInput:
            std::println(stderr, "{}", result.diagnostic);
            return (int)ExitCode::kNoInput;
        case StatsError::kIOError:
            std::println(stderr, "{}", result.diagnostic);
            return (int)ExitCode::kIOErr;
        default:
            std::println(stderr, "{}", result.diagnostic);
            return (int)ExitCode::kData;
        }
    }

    ppm_print_stats(*result.value, std::cout);
    return (int)ExitCode::kOk;
}
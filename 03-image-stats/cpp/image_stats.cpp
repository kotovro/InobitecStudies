#include "ppm_stats.hpp"

#include "../../common/cpp/exit_codes.hpp"
#include "../../common/cpp/version.hpp"

#include <cstdlib>
#include <iostream>
#include <print>
#include <string_view>

using namespace raster::common;
using namespace raster::stats;

namespace {

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

} // namespace

int main(int argc, char** argv) {
    if (argc >= 2 && std::string_view(argv[1]) == "--help") {
        print_usage();
        return (int)ExitCode::kOk;
    }

    if (argc >= 2 && std::string_view(argv[1]) == "--version") {
        print_version();
        return (int)ExitCode::kOk;
    }

    auto result = Image::read(std::cin);
    if (!result.value) {
        std::println(stderr, "{}", result.diagnostic);
        switch (result.value.error()) {
        case PpmReadError::kEmptyInput:
            return (int)ExitCode::kNoInput;
        case PpmReadError::kIOError:
            return (int)ExitCode::kIOErr;
        default:
            return (int)ExitCode::kData;
        }
    }

    Stats stats = compute_stats(*result.value);
    ppm_print_stats(stats, std::cout);
    return (int)ExitCode::kOk;
}

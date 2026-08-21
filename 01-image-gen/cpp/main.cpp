#include "parse_args.hpp"
#include "patterns.hpp"

#include "../../common/cpp/ppm_io.hpp"
#include "../../common/cpp/version.hpp"

#include <iostream>
#include <print>
#include <random>
#include <utility>

using namespace raster::common;
using namespace raster::gen;

namespace {

void generate_ppm(const Args& args) {
    PpmWriter writer(std::cout, args.size, args.size);
    std::mt19937 rng(args.seed ? args.seed : 42);
    for (int32_t y = 0; y < args.size; ++y) {
        for (int32_t x = 0; x < args.size; ++x) {
            RGB c{};
            switch (args.pattern) {
                using enum Pattern;
            case Gradient:
                c = gradient_pixel(x, y, args.size);
                break;
            case Checker:
                c = checker_pixel(x, y);
                break;
            case Radial:
                c = radial_pixel(x, y, args.size);
                break;
            case Random:
                c = random_pixel(rng);
                break;
            }
            writer.put(c.r, c.g, c.b);
        }
    }
}

void print_usage() {
    std::println("Использование: gen_image <N> [pattern]");
    std::println("               gen_image --size N [--seed S]");
    std::println();
    std::println("Позиционный режим:");
    std::println("  N        сторона квадрата (1-512)");
    std::println("  pattern  gradient | checker | radial (по умолчанию gradient)");
    std::println();
    std::println("Массовая генерация:");
    std::println("  --size N    сторона (без ограничения 512)");
    std::println("  --seed S    seed ГПСЧ (по умолчанию 42)");
    std::println();
    std::println("Опции:");
    std::println("  --help      показать справку");
    std::println("  --version   показать версию");
}

void print_version() { std::println("gen_image {}", kVersion); }

} // namespace

int main(int argc, char** argv) {
    auto result = parse_args(argc, argv);
    if (!result) [[unlikely]] {
        switch (result.error()) {
        case ParseError::kNoArg:
            std::println(stderr, "N не указано. Использование: gen_image <N> [pattern]");
            return std::to_underlying(ExitCode::kUsage);
        case ParseError::kBadNumber:
            std::println(stderr, "N должно быть целым числом; получено: {}",
                         argc >= 2 ? argv[1] : "");
            return std::to_underlying(ExitCode::kUsage);
        case ParseError::kBadPattern:
            std::println(stderr, "Неизвестный паттерн: {}. Допустимые: gradient, checker, radial",
                         argc >= 3 ? argv[2] : "");
            return std::to_underlying(ExitCode::kUsage);
        case ParseError::kBadSeed:
            std::println(stderr, "seed должно быть целым числом; получено: {}",
                         argc >= 5 ? argv[4] : "");
            return std::to_underlying(ExitCode::kUsage);
        }
    }

    if (result->request == ParseRequest::kHelp) [[unlikely]] {
        print_usage();
        return std::to_underlying(ExitCode::kOk);
    }

    if (result->request == ParseRequest::kVersion) [[unlikely]] {
        print_version();
        return std::to_underlying(ExitCode::kOk);
    }

    const Args& args = result->args;
    if (args.size < 1) [[unlikely]] {
        std::println(stderr, "размер должен быть положительным; получено: {}", args.size);
        return std::to_underlying(ExitCode::kUsage);
    }

    if (args.pattern != Pattern::Random && args.size > 512) [[unlikely]] {
        std::println(stderr, "N должно быть в [1; 512]; получено: {}", args.size);
        return std::to_underlying(ExitCode::kUsage);
    }

    generate_ppm(args);
    return std::to_underlying(ExitCode::kOk);
}

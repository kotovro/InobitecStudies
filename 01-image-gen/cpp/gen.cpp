#include "hsv_to_rgb.hpp"
#include "parse_args.hpp"

#include "../../common/cpp/ppm_io.hpp"
#include "../../common/cpp/version.hpp"

#include <cmath>
#include <cstdint>
#include <expected>
#include <iostream>
#include <print>
#include <random>

void draw_random(PpmWriter& w, int32_t size, uint32_t seed) {
    std::mt19937 rng(seed ? seed : 42);
    std::uniform_int_distribution dist(0, 255);
    for (int32_t y = 0; y < size; ++y) {
        for (int32_t x = 0; x < size; ++x) {
            int32_t r = dist(rng);
            int32_t g = dist(rng);
            int32_t b = dist(rng);
            w.put(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
        }
    }
}

void draw_gradient(PpmWriter& w, int32_t size) {
    int32_t max_coord = (size == 1) ? 1 : (size - 1);
    for (int32_t y = 0; y < size; ++y) {
        for (int32_t x = 0; x < size; ++x) {
            int32_t r = x * 255 / max_coord;
            int32_t g = (max_coord - y) * 255 / max_coord;
            int32_t b = 0;
            w.put(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
        }
    }
}

void draw_checker(PpmWriter& w, int32_t size) {
    for (int32_t y = 0; y < size; ++y) {
        for (int32_t x = 0; x < size; ++x) {
            int32_t v = ((x + y) % 2 == 0) ? 255 : 0;
            w.put(static_cast<uint8_t>(v), static_cast<uint8_t>(v), static_cast<uint8_t>(v));
        }
    }
}

void draw_radial(PpmWriter& w, int32_t size) {
    double cx = (size - 1) / 2.0;
    double cy = (size - 1) / 2.0;
    double max_dist = std::sqrt(cx * cx + cy * cy);
    for (int32_t y = 0; y < size; ++y) {
        for (int32_t x = 0; x < size; ++x) {
            double dx = x - cx;
            double dy = y - cy;
            double dist = std::sqrt(dx * dx + dy * dy);

            double hue = max_dist > 0 ? (dist / max_dist) * 360.0 : 0.0;
            RGB color = hsv_to_rgb(hue, 1.0, 1.0);
            w.put(color.r, color.g, color.b);
        }
    }
}

void generate_ppm(const Args& args) {
    PpmWriter writer(std::cout, args.size, args.size);
    switch (args.pattern) {
        using enum Pattern;
    case Gradient:
        draw_gradient(writer, args.size);
        break;
    case Checker:
        draw_checker(writer, args.size);
        break;
    case Radial:
        draw_radial(writer, args.size);
        break;
    case Random:
        draw_random(writer, args.size, args.seed);
        break;
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

int main(int argc, char** argv) {
    auto result = parse_args(argc, argv);
    if (!result) [[unlikely]] {
        switch (result.error()) {
        case ParseError::kNoArg:
            std::println(stderr, "N не указано. Использование: gen_image <N> [pattern]");
            return (int)ExitCode::kUsage;
        case ParseError::kBadNumber:
            std::println(stderr, "N должно быть целым числом; получено: {}",
                         argc >= 2 ? argv[1] : "");
            return (int)ExitCode::kUsage;
        case ParseError::kBadPattern:
            std::println(stderr, "Неизвестный паттерн: {}. Допустимые: gradient, checker, radial",
                         argc >= 3 ? argv[2] : "");
            return (int)ExitCode::kUsage;
        case ParseError::kBadSeed:
            std::println(stderr, "seed должно быть целым числом; получено: {}",
                         argc >= 5 ? argv[4] : "");
            return (int)ExitCode::kUsage;
        }
    }

    if (result->request == ParseRequest::kHelp) [[unlikely]] {
        print_usage();
        return (int)ExitCode::kOk;
    }

    if (result->request == ParseRequest::kVersion) [[unlikely]] {
        print_version();
        return (int)ExitCode::kOk;
    }

    const Args& args = result->args;
    if (args.size < 1) [[unlikely]] {
        std::println(stderr, "размер должен быть положительным; получено: {}", args.size);
        return (int)ExitCode::kUsage;
    }

    if (args.pattern != Pattern::Random && args.size > 512) [[unlikely]] {
        std::println(stderr, "N должно быть в [1; 512]; получено: {}", args.size);
        return (int)ExitCode::kUsage;
    }

    generate_ppm(args);
    return (int)ExitCode::kOk;
}
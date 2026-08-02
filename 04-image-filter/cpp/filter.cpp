#include "filter.hpp"

#include <charconv>
#include <cstring>
#include <print>
#include <string_view>

Pixel pixel_to_grayscale(const Pixel& p) {
    double y = luma(p.r, p.g, p.b);
    uint8_t yb = static_cast<uint8_t>(y + 0.5);
    return {yb, yb, yb};
}

Pixel pixel_threshold(const Pixel& p, int threshold) {
    double y = luma(p.r, p.g, p.b);
    return (y > threshold) ? Pixel{255, 255, 255} : Pixel{0, 0, 0};
}

void apply_grayscale(Image& img) {
    for (auto& pixel : img.pixels())
        pixel = pixel_to_grayscale(pixel);
}

void apply_threshold(Image& img, int threshold) {
    for (auto& pixel : img.pixels())
        pixel = pixel_threshold(pixel, threshold);
}

std::optional<FilterArgs> parse_filter_args(int argc, char** argv) {
    if (argc < 2) {
        std::println(stderr, "использование: image_filter --grayscale | --threshold T");
        return std::nullopt;
    }

    std::string_view arg(argv[1]);

    if (arg == "--grayscale") {
        if (argc > 2) {
            std::println(stderr, "--grayscale не принимает аргументов");
            return std::nullopt;
        }
        return FilterArgs{FilterMode::kGrayscale};
    }

    if (arg == "--threshold") {
        if (argc < 3) {
            std::println(stderr, "--threshold требует аргумента T");
            return std::nullopt;
        }

        int t{};
        const char* val = argv[2];
        auto [ptr, ec] = std::from_chars(val, val + std::strlen(val), t);
        if (ec != std::errc{} || *ptr != '\0') {
            std::println(stderr, "T должен быть целым числом; получено: {}", val);
            return std::nullopt;
        }

        if (t < 0 || t > 255) {
            std::println(stderr, "T должен быть в [0; 255]; получено: {}", t);
            return std::nullopt;
        }

        return FilterArgs{FilterMode::kThreshold, t};
    }

    std::println(stderr, "неизвестный аргумент: {}. используйте --grayscale или --threshold T",
                 arg);
    return std::nullopt;
}

int run_filter(int argc, char** argv, std::istream& input, std::ostream& output) {
    auto args = parse_filter_args(argc, argv);
    if (!args)
        return static_cast<int>(ExitCode::kUsage);

    auto result = Image::read(input);
    if (!result.value.has_value()) {
        std::println(stderr, "{}", result.diagnostic);
        switch (result.value.error()) {
        case PpmReadError::kEmptyInput:
            return static_cast<int>(ExitCode::kNoInput);
        case PpmReadError::kIOError:
            return static_cast<int>(ExitCode::kIOErr);
        default:
            return static_cast<int>(ExitCode::kData);
        }
    }

    auto& image = *result.value;

    switch (args->mode) {
    case FilterMode::kGrayscale:
        apply_grayscale(image);
        break;
    case FilterMode::kThreshold:
        apply_threshold(image, args->threshold);
        break;
    }

    PpmWriter writer(output, image.width(), image.height());
    for (const auto& pixel : image.pixels())
        writer.put(pixel.r, pixel.g, pixel.b);

    return static_cast<int>(ExitCode::kOk);
}
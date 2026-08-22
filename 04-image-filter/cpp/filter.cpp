#include "filter.hpp"

#include "../../common/cpp/luma.hpp"
#include "../../common/cpp/version.hpp"

#include <charconv>
#include <ostream>
#include <print>
#include <string_view>
#include <utility>

using namespace raster::common;

namespace raster::filter {

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

std::optional<FilterParseResult> parse_filter_args(std::span<const std::string_view> args) {
    if (args.size() < 2) {
        std::println(stderr, "использование: filter --grayscale | --threshold T");
        return std::nullopt;
    }

    std::string_view arg = args[1];

    if (arg == "--help")
        return FilterParseResult{.request = FilterRequest::kHelp};

    if (arg == "--version")
        return FilterParseResult{.request = FilterRequest::kVersion};

    if (arg == "--grayscale") {
        if (args.size() > 2) {
            std::println(stderr, "--grayscale не принимает аргументов");
            return std::nullopt;
        }
        return FilterParseResult{.args = FilterArgs{FilterMode::kGrayscale}};
    }

    if (arg == "--threshold") {
        if (args.size() < 3) {
            std::println(stderr, "--threshold требует аргумента T");
            return std::nullopt;
        }

        int t{};
        auto [ptr, ec] = std::from_chars(args[2].data(), args[2].data() + args[2].size(), t);
        if (ec != std::errc{} || ptr != args[2].data() + args[2].size()) {
            std::println(stderr, "T должен быть целым числом; получено: {}", args[2]);
            return std::nullopt;
        }

        if (t < 0 || t > 255) {
            std::println(stderr, "T должен быть в [0; 255]; получено: {}", t);
            return std::nullopt;
        }

        return FilterParseResult{.args = FilterArgs{FilterMode::kThreshold, t}};
    }

    std::println(stderr, "неизвестный аргумент: {}. используйте --grayscale или --threshold T",
                 arg);
    return std::nullopt;
}

void print_filter_usage(std::ostream& os) {
    std::println(os, "Использование: filter --grayscale | --threshold T");
    std::println(os);
    std::println(os, "Читает PPM P3 из stdin, пишет валидный PPM в stdout.");
    std::println(os);
    std::println(os, "Режимы:");
    std::println(os, "  --grayscale     конверсия в оттенки серого по luma");
    std::println(os, "  --threshold T   бинаризация по порогу яркости (0 <= T <= 255)");
    std::println(os);
    std::println(os, "Опции:");
    std::println(os, "  --help          показать справку");
    std::println(os, "  --version       показать версию");
}

void print_filter_version(std::ostream& os) { std::println(os, "filter {}", kVersion); }

int run_filter(std::span<const std::string_view> args, std::istream& input, std::ostream& output) {
    auto parsed = parse_filter_args(args);
    if (!parsed)
        return std::to_underlying(ExitCode::kUsage);

    if (parsed->request == FilterRequest::kHelp) {
        print_filter_usage(output);
        return std::to_underlying(ExitCode::kOk);
    }

    if (parsed->request == FilterRequest::kVersion) {
        print_filter_version(output);
        return std::to_underlying(ExitCode::kOk);
    }

    const FilterArgs& filter_args = parsed->args;
    auto result = Image::read(input);
    if (!result.value.has_value()) {
        std::println(stderr, "{}", result.diagnostic);
        switch (result.value.error()) {
        case PpmReadError::kEmptyInput:
            return std::to_underlying(ExitCode::kNoInput);
        case PpmReadError::kIOError:
            return std::to_underlying(ExitCode::kIOErr);
        default:
            return std::to_underlying(ExitCode::kData);
        }
    }

    auto& image = *result.value;

    switch (filter_args.mode) {
    case FilterMode::kGrayscale:
        apply_grayscale(image);
        break;
    case FilterMode::kThreshold:
        apply_threshold(image, filter_args.threshold);
        break;
    }

    PpmWriter writer(output, image.width(), image.height());
    auto header = writer.putHeader();
    if (!header.value) {
        std::println(stderr, "{}", header.diagnostic);
        return std::to_underlying(ExitCode::kIOErr);
    }
    for (const auto& pixel : image.pixels()) {
        auto res = writer.put(pixel.r, pixel.g, pixel.b);
        if (!res.value) {
            std::println(stderr, "{}", res.diagnostic);
            switch (res.value.error()) {
            case PpmWriteError::kIOError:
                return std::to_underlying(ExitCode::kIOErr);
            case PpmWriteError::kTooManyPixels:
                return std::to_underlying(ExitCode::kData);
            }
        }
    }

    return std::to_underlying(ExitCode::kOk);
}

} // namespace raster::filter

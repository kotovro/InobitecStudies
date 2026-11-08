#include "ppm_stats.hpp"

#include "../../common/cpp/exit_codes.hpp"
#include "../../common/cpp/luma.hpp"

#include <cerrno>
#include <cmath>
#include <cstdint>
#include <format>
#include <iostream>
#include <ostream>
#include <print>
#include <system_error>
#include <utility>

using namespace raster::common;

namespace raster::stats {

Stats compute_stats(const Image& img) {
    Stats s{};
    s.width = img.width();
    s.height = img.height();
    s.max_val = img.max_val();

    bool first_pixel = true;

    for (const auto& p : img.pixels()) {
        int32_t r = p.r;
        int32_t g = p.g;
        int32_t b = p.b;

        s.total_r += r;
        s.total_g += g;
        s.total_b += b;

        double y = luma(r, g, b);

        if (first_pixel) {
            s.y_min = y;
            s.y_max = y;
            first_pixel = false;
        } else {
            if (y < s.y_min)
                s.y_min = y;
            if (y > s.y_max)
                s.y_max = y;
        }

        int32_t bin = (int32_t)(y * 8.0 / (s.max_val + 1));
        if (bin >= 8)
            bin = 7;
        ++s.histogram[bin];
        ++s.pixel_count;
    }

    return s;
}

StatsWriteResult ppm_print_stats(const Stats& s, std::ostream& os) {
    int32_t avg_r = (int32_t)((double)s.total_r / s.pixel_count + 0.5);
    int32_t avg_g = (int32_t)((double)s.total_g / s.pixel_count + 0.5);
    int32_t avg_b = (int32_t)((double)s.total_b / s.pixel_count + 0.5);

    errno = 0;
    std::println(os, "{}x{}", s.width, s.height);
    std::println(os, "пикселей: {}", s.pixel_count);
    std::println(os, "средний цвет: {:3d} {:3d} {:3d}", avg_r, avg_g, avg_b);
    std::println(os, "мин. яркость: {:.1f}", s.y_min);
    std::println(os, "макс. яркость: {:.1f}", s.y_max);
    std::println(os, "гистограмма яркости:");

    for (int32_t i = 0; i < 8; ++i) {
        double bin_width = (double)(s.max_val + 1) / 8.0;
        int32_t lo = (int32_t)(i * bin_width);
        int32_t hi = (i == 7) ? s.max_val : (int32_t)((i + 1) * bin_width) - 1;
        std::println(os, "  [{:3d},{:3d}]: {}", lo, hi, s.histogram[i]);
    }

    os.flush();
    if (os.bad() || os.fail()) {
        int e = errno;
        std::string diagnostic;
        if (e != 0) {
            diagnostic = std::format("сбой записи отчёта: {} (errno {})",
                                     std::error_code(e, std::generic_category()).message(), e);
        } else {
            diagnostic = "сбой записи отчёта: неизвестная ошибка ввода-вывода";
        }
        return StatsWriteResult{std::unexpected(StatsWriteError::kIOError), std::move(diagnostic)};
    }

    return StatsWriteResult{};
}

int run_stats(std::istream& input, std::ostream& output) {
    auto result = Image::read(input);
    if (!result.value) {
        std::println(stderr, "{}", result.diagnostic);
        switch (result.value.error()) {
        case PpmReadError::kEmptyInput:
            return std::to_underlying(ExitCode::kNoInput);
        case PpmReadError::kIOError:
            return std::to_underlying(ExitCode::kIOErr);
        case PpmReadError::kAllocError:
            return std::to_underlying(ExitCode::kData);
        default:
            return std::to_underlying(ExitCode::kData);
        }
    }

    Stats stats = compute_stats(*result.value);
    auto printed = ppm_print_stats(stats, output);
    if (!printed.value) {
        std::println(stderr, "{}", printed.diagnostic);
        return std::to_underlying(ExitCode::kIOErr);
    }

    return std::to_underlying(ExitCode::kOk);
}

} // namespace raster::stats

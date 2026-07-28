#include "ppm_stats.hpp"

#include <cmath>
#include <expected>
#include <iostream>
#include <limits>
#include <print>
#include <system_error>

// -------------------------------------------------------------------
// Вспомогательные функции
// -------------------------------------------------------------------

double luma(int r, int g, int b) { return 0.299 * r + 0.587 * g + 0.114 * b; }

// -------------------------------------------------------------------
// Публичные функции
// -------------------------------------------------------------------

StatsResult ppm_read_stats(std::istream& is) {
    Stats stats{};
    int line_num = 1;
    enum class Phase { kHeader, kData } phase = Phase::kHeader;

    auto err = [&](StatsError e, std::string msg) -> StatsResult {
        return StatsResult{std::unexpected(e), std::move(msg)};
    };

    // skip_ws: пропускает пробелы. В фазе HEADER также целиком пропускает
    // #-строки.
    auto skip_ws = [&]() -> bool {
        while (true) {
            is >> std::ws;
            if (is.eof())
                return false;
            if (phase == Phase::kHeader && is.peek() == '#') {
                is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                ++line_num;
                continue;
            }
            break;
        }
        return true;
    };

    // ---- 1. Magic ----
    is >> std::ws;
    if (is.eof())
        return err(StatsError::kEmptyInput, "нет входных данных");

    std::string magic;
    is >> magic;
    if (is.fail())
        return err(StatsError::kIOError,
                   std::format("сбой ввода: {}", std::generic_category().message(errno)));

    if (magic != "P3")
        return err(StatsError::kBadMagic,
                   std::format("строка {}: ожидалось 'P3', получено: '{}'", line_num, magic));

    // ---- 2. Width, Height, Maxval ----
    auto read_header_int = [&](long long& out, std::string_view /*label*/) -> bool {
        if (!skip_ws())
            return false;
        is >> out;
        if (is.fail()) {
            is.clear();
            return false;
        }
        return true;
    };

    {
        long long w, h, m;
        if (!read_header_int(w, "width"))
            return err(StatsError::kBadNumber,
                       std::format("строка {}: не удалось прочитать ширину", line_num));
        if (w <= 0)
            return err(
                StatsError::kBadNumber,
                std::format("строка {}: ширина должна быть положительным целым; получено: {}",
                            line_num, w));
        if (!read_header_int(h, "height"))
            return err(StatsError::kBadNumber,
                       std::format("строка {}: не удалось прочитать высоту", line_num));
        if (h <= 0)
            return err(
                StatsError::kBadNumber,
                std::format("строка {}: высота должна быть положительным целым; получено: {}",
                            line_num, h));
        if (!read_header_int(m, "maxval"))
            return err(StatsError::kBadNumber,
                       std::format("строка {}: не удалось прочитать максимальное значение канала",
                                   line_num));
        if (m != 255)
            return err(StatsError::kBadNumber,
                       std::format("строка {}: максимальное значение канала должно быть 255; "
                                   "получено: {}",
                                   line_num, m));

        stats.width = (int)w;
        stats.height = (int)h;
        stats.max_val = (int)m;
    }

    // ---- 3. Попиксельные данные (PH_DATA) ----
    phase = Phase::kData;
    long long total_pixels = (long long)stats.width * stats.height;
    bool first_pixel = true;

    for (long long i = 0; i < total_pixels; ++i) {
        skip_ws();
        if (is.eof())
            return err(StatsError::kTooFewPixels,
                       std::format("строка {}: неожиданный конец файла после {} пикселей "
                                   "(ожидалось {})",
                                   line_num, stats.pixel_count, total_pixels));

        if (is.peek() == '#')
            return err(StatsError::kBadNumber,
                       std::format("строка {}: символ '#' не допускается в данных", line_num));

        int r, g, b;
        is >> r >> g >> b;
        if (is.fail())
            return err(StatsError::kBadNumber, std::format("строка {}: ожидалось число", line_num));

        if (r < 0 || r > stats.max_val || g < 0 || g > stats.max_val || b < 0 || b > stats.max_val)
            return err(StatsError::kChannelRange,
                       std::format("строка {}: значение канала должно быть в [0; {}]; "
                                   "получено: {} {} {}",
                                   line_num, stats.max_val, r, g, b));

        stats.total_r += r;
        stats.total_g += g;
        stats.total_b += b;

        double y = luma(r, g, b);

        if (first_pixel) {
            stats.y_min = y;
            stats.y_max = y;
            first_pixel = false;
        } else {
            if (y < stats.y_min)
                stats.y_min = y;
            if (y > stats.y_max)
                stats.y_max = y;
        }

        int bin = (int)(y * 8.0 / (stats.max_val + 1));
        if (bin >= 8)
            bin = 7;
        ++stats.histogram[bin];
        ++stats.pixel_count;
    }

    // ---- 4. Проверка на лишние данные ----
    is >> std::ws;
    if (!is.eof()) {
        if (is.peek() == '#')
            return err(StatsError::kBadNumber,
                       std::format("строка {}: символ '#' не допускается в данных", line_num));

        char extra;
        is >> extra;
        if (!is.eof())
            return err(StatsError::kTooManyPixels,
                       std::format("строка {}: лишние данные после {} пикселей", line_num,
                                   stats.pixel_count));
    }

    if (is.bad())
        return err(StatsError::kIOError,
                   std::format("сбой ввода: {}", std::generic_category().message(errno)));

    return StatsResult{stats, {}};
}

void ppm_print_stats(const Stats& s, std::ostream& os) {
    int avg_r = (int)((double)s.total_r / s.pixel_count + 0.5);
    int avg_g = (int)((double)s.total_g / s.pixel_count + 0.5);
    int avg_b = (int)((double)s.total_b / s.pixel_count + 0.5);

    std::println(os, "{}x{}", s.width, s.height);
    std::println(os, "пикселей: {}", s.pixel_count);
    std::println(os, "средний цвет: {} {} {}", avg_r, avg_g, avg_b);
    std::println(os, "мин. яркость: {:.1f}", s.y_min);
    std::println(os, "макс. яркость: {:.1f}", s.y_max);
    std::println(os, "гистограмма яркости:");

    for (int i = 0; i < 8; ++i) {
        double bin_width = (double)(s.max_val + 1) / 8.0;
        int lo = (int)(i * bin_width);
        int hi = (i == 7) ? s.max_val : (int)((i + 1) * bin_width) - 1;
        std::println(os, "  [{:3d},{:3d}]: {}", lo, hi, s.histogram[i]);
    }
}
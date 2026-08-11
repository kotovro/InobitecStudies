#include "ppm_stats.hpp"

#include <cmath>
#include <cstdint>
#include <ostream>
#include <print>

double luma(int32_t r, int32_t g, int32_t b) { return 0.299 * r + 0.587 * g + 0.114 * b; }

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

void ppm_print_stats(const Stats& s, std::ostream& os) {
    int32_t avg_r = (int32_t)((double)s.total_r / s.pixel_count + 0.5);
    int32_t avg_g = (int32_t)((double)s.total_g / s.pixel_count + 0.5);
    int32_t avg_b = (int32_t)((double)s.total_b / s.pixel_count + 0.5);

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
}
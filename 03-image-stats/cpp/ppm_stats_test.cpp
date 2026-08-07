#include "ppm_stats.hpp"

#include <cstdint>
#include <cstdlib>
#include <locale>
#include <optional>
#include <print>
#include <sstream>
#include <string>
#include <string_view>

double luma(int32_t r, int32_t g, int32_t b);

static int failed = 0;

static void check(bool cond, std::string_view name) {
    if (!cond) {
        std::println(stderr, "FAIL: {}", name);
        ++failed;
    } else {
        std::println("PASS: {}", name);
    }
}

static void check_stats(const Stats& s, int32_t w, int32_t h, int64_t px, int64_t tr, int64_t tg,
                        int64_t tb) {
    check(s.width == w, "width");
    check(s.height == h, "height");
    check(s.pixel_count == px, "pixel_count");
    check(s.total_r == tr, "total_r");
    check(s.total_g == tg, "total_g");
    check(s.total_b == tb, "total_b");
}

static std::optional<Stats> read_stats_from(std::string_view ppm) {
    auto ss = std::istringstream(std::string(ppm));
    auto r = Image::read(ss);
    if (!r.value.has_value())
        return std::nullopt;
    return compute_stats(*r.value);
}

// Statistics accuracy

static void test_stats_known() {
    auto s = read_stats_from("P3\n2 2\n255\n"
                             "0 0 0 255 0 0\n"
                             "0 255 0 0 0 255\n");
    check(s.has_value(), "known image -> ok");
    if (!s.has_value())
        return;

    check_stats(*s, 2, 2, 4, 255, 255, 255);
    check(s->y_min >= 0.0 && s->y_min <= 0.1, "y_min ~ 0.0");
    check(s->y_max >= 149.5 && s->y_max <= 149.9, "y_max ~ 149.7");
    check(s->histogram[0] == 2, "hist[0] == 2");
    check(s->histogram[2] == 1, "hist[2] == 1");
    check(s->histogram[4] == 1, "hist[4] == 1");
}

static void test_stats_comments() {
    auto s = read_stats_from("P3\n# width\n2 2\n# before maxval\n255\n"
                             "0 0 0\n0 0 0\n0 0 0\n0 0 0\n");
    check(s.has_value(), "comments in header -> ok");
    if (s.has_value())
        check_stats(*s, 2, 2, 4, 0, 0, 0);
}

// ---- luma unit tests ----

static void test_luma() {
    auto y = ::luma(255, 0, 0);
    check(y >= 76.2 && y <= 76.3, "luma(255,0,0) ~ 76.245");
    y = ::luma(0, 255, 0);
    check(y >= 149.6 && y <= 149.7, "luma(0,255,0) ~ 149.685");
    y = ::luma(0, 0, 255);
    check(y >= 29.0 && y <= 29.1, "luma(0,0,255) ~ 29.07");
    y = ::luma(255, 255, 255);
    check(y >= 254.9 && y <= 255.1, "luma(255,255,255) == 255.0");
    y = ::luma(0, 0, 0);
    check(y >= -0.1 && y <= 0.1, "luma(0,0,0) == 0.0");
    y = ::luma(128, 128, 128);
    check(y >= 127.9 && y <= 128.1, "luma(128,128,128) ~ 128.0");
}

// ---- main ----

int main() {
    std::setlocale(LC_ALL, "Russian_Russia.1251");

    std::println("--- compute_stats tests ---");
    test_stats_known();
    test_stats_comments();

    std::println("-- luma --");
    test_luma();

    std::println("---");
    if (failed > 0)
        std::println(stderr, "{} tests FAILED", failed);
    else
        std::println("All tests PASSED");

    return failed > 0 ? 1 : 0;
}
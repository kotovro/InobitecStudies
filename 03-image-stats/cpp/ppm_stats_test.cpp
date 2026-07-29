#include "ppm_stats.hpp"

#include <cstdint>
#include <cstdlib>
#include <locale>
#include <print>
#include <sstream>
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

// Header tests

static void test_empty_input() {
    auto ss = std::istringstream("");
    auto r = ppm_read_stats(ss);
    check(!r.value.has_value(), "empty input -> error");
    if (!r.value.has_value())
        check(r.value.error() == StatsError::kEmptyInput, "empty input -> kEmptyInput");
}

static void test_magic_bad() {
    auto ss = std::istringstream("P5\n1 1\n255\n0\n");
    auto r = ppm_read_stats(ss);
    check(!r.value.has_value(), "P5 -> error");
    if (!r.value.has_value())
        check(r.value.error() == StatsError::kBadMagic, "P5 -> kBadMagic");
}

static void test_magic_truncated() {
    auto ss = std::istringstream("P");
    auto r = ppm_read_stats(ss);
    check(!r.value.has_value(), "just P -> error");
    if (!r.value.has_value())
        check(r.value.error() == StatsError::kBadMagic, "just P -> kBadMagic");
}

static void test_magic_ok() {
    auto ss = std::istringstream("P3\n1 1\n255\n128 128 128\n");
    auto r = ppm_read_stats(ss);
    check(r.value.has_value(), "valid P3 -> ok");
    if (r.value.has_value()) {
        check_stats(*r.value, 1, 1, 1, 128, 128, 128);
    }
}

static void test_maxval_not_255() {
    auto ss = std::istringstream("P3\n1 1\n100\n0 0 0\n");
    auto r = ppm_read_stats(ss);
    check(!r.value.has_value(), "maxval=100 -> error");
}

static void test_width_zero() {
    auto ss = std::istringstream("P3\n0 1\n255\n0 0 0\n");
    auto r = ppm_read_stats(ss);
    check(!r.value.has_value(), "width=0 -> error");
    if (!r.value.has_value())
        check(r.value.error() == StatsError::kBadNumber, "width=0 -> kBadNumber");
}

static void test_comments_header() {
    auto ss = std::istringstream("P3\n# width\n2 2\n# before maxval\n255\n"
                                 "0 0 0\n0 0 0\n0 0 0\n0 0 0\n");
    auto r = ppm_read_stats(ss);
    check(r.value.has_value(), "comments in header -> ok");
    if (r.value.has_value())
        check_stats(*r.value, 2, 2, 4, 0, 0, 0);
}

// Pixel data tests

static void test_channel_out_of_range() {
    auto ss = std::istringstream("P3\n1 1\n255\n256 0 0\n");
    auto r = ppm_read_stats(ss);
    check(!r.value.has_value(), "channel 256 -> error");
    if (!r.value.has_value())
        check(r.value.error() == StatsError::kChannelRange, "channel 256 -> kChannelRange");
}

static void test_channel_negative() {
    auto ss = std::istringstream("P3\n1 1\n255\n-1 0 0\n");
    auto r = ppm_read_stats(ss);
    check(!r.value.has_value(), "channel -1 -> error");
    if (!r.value.has_value())
        check(r.value.error() == StatsError::kChannelRange, "channel -1 -> kChannelRange");
}

static void test_not_a_number() {
    auto ss = std::istringstream("P3\n1 1\n255\nx 0 0\n");
    auto r = ppm_read_stats(ss);
    check(!r.value.has_value(), "not a number -> error");
    if (!r.value.has_value())
        check(r.value.error() == StatsError::kBadNumber, "not a number -> kBadNumber");
}

static void test_hash_in_data() {
    auto ss = std::istringstream("P3\n1 1\n255\n0 0 0 # trailing\n");
    auto r = ppm_read_stats(ss);
    check(!r.value.has_value(), "hash in data -> error");
    if (!r.value.has_value())
        check(r.value.error() == StatsError::kBadNumber, "hash in data -> kBadNumber");
}

static void test_too_many_pixels() {
    auto ss = std::istringstream("P3\n1 1\n255\n0 0 0 255 255 255\n");
    auto r = ppm_read_stats(ss);
    check(!r.value.has_value(), "extra pixel -> error");
    if (!r.value.has_value())
        check(r.value.error() == StatsError::kTooManyPixels, "extra -> kTooManyPixels");
}

static void test_too_few_pixels() {
    auto ss = std::istringstream("P3\n2 2\n255\n0 0 0 255 0 0 0 255 0\n");
    auto r = ppm_read_stats(ss);
    check(!r.value.has_value(), "missing pixel -> error");
    if (!r.value.has_value())
        check(r.value.error() == StatsError::kTooFewPixels, "missing -> kTooFewPixels");
}

// Statistics accuracy

static void test_stats_known() {
    auto ss = std::istringstream("P3\n2 2\n255\n"
                                 "0 0 0 255 0 0\n"
                                 "0 255 0 0 0 255\n");
    auto r = ppm_read_stats(ss);
    check(r.value.has_value(), "known image -> ok");
    if (r.value.has_value()) {
        const auto& s = *r.value;
        check_stats(s, 2, 2, 4, 255, 255, 255);
        check(s.y_min >= 0.0 && s.y_min <= 0.1, "y_min ~ 0.0");
        check(s.y_max >= 149.5 && s.y_max <= 149.9, "y_max ~ 149.7");
        check(s.histogram[0] == 2, "hist[0] == 2");
        check(s.histogram[2] == 1, "hist[2] == 1");
        check(s.histogram[4] == 1, "hist[4] == 1");
    }
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

    std::println("--- ppm_read_stats tests ---");

    std::println("-- header errors --");
    test_empty_input();
    test_magic_bad();
    test_magic_truncated();
    test_magic_ok();
    test_maxval_not_255();
    test_width_zero();
    test_comments_header();

    std::println("-- pixel data errors --");
    test_channel_out_of_range();
    test_channel_negative();
    test_not_a_number();
    test_hash_in_data();
    test_too_many_pixels();
    test_too_few_pixels();

    std::println("-- luma --");
    test_luma();

    std::println("-- statistics --");
    test_stats_known();

    std::println("---");
    if (failed > 0)
        std::println(stderr, "{} tests FAILED", failed);
    else
        std::println("All tests PASSED");

    return failed > 0 ? 1 : 0;
}
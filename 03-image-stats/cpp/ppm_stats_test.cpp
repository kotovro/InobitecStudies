#include "ppm_stats.hpp"

#include <cstdint>
#include <cstdlib>
#include <optional>
#include <print>
#include <sstream>
#include <string>
#include <string_view>

using namespace raster::common;
using namespace raster::stats;

namespace {

int failed = 0;

void check(bool cond, std::string_view name) {
    if (!cond) {
        std::println(stderr, "FAIL: {}", name);
        ++failed;
    } else {
        std::println("PASS: {}", name);
    }
}

void check_stats(const Stats& s, int32_t w, int32_t h, int64_t px, int64_t tr, int64_t tg,
                 int64_t tb) {
    check(s.width == w, "width");
    check(s.height == h, "height");
    check(s.pixel_count == px, "pixel_count");
    check(s.total_r == tr, "total_r");
    check(s.total_g == tg, "total_g");
    check(s.total_b == tb, "total_b");
}

std::optional<Stats> read_stats_from(std::string_view ppm) {
    auto ss = std::istringstream(std::string(ppm));
    auto r = Image::read(ss);
    if (!r.value.has_value())
        return std::nullopt;
    return compute_stats(*r.value);
}

// Statistics accuracy

void test_stats_known() {
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

void test_stats_comments() {
    auto s = read_stats_from("P3\n# width\n2 2\n# before maxval\n255\n"
                             "0 0 0\n0 0 0\n0 0 0\n0 0 0\n");
    check(s.has_value(), "comments in header -> ok");
    if (s.has_value())
        check_stats(*s, 2, 2, 4, 0, 0, 0);
}

// ---- main ----

} // namespace

int main() {
    std::println("--- compute_stats tests ---");
    test_stats_known();
    test_stats_comments();

    std::println("---");
    if (failed > 0)
        std::println(stderr, "{} tests FAILED", failed);
    else
        std::println("All tests PASSED");

    return failed > 0 ? 1 : 0;
}
#include "filter.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <print>
#include <string>
#include <string_view>
#include <utility>

using namespace raster::common;
using namespace raster::filter;

namespace {

int failed = 0;

void check(bool cond, const char* test_name) {
    if (!cond) {
        std::println(stderr, "FAIL: {}", test_name);
        ++failed;
    } else {
        std::println("PASS: {}", test_name);
    }
}

void check_pixel(const Pixel& pixel, uint8_t er, uint8_t eg, uint8_t eb, const char* test_name) {
    if (pixel.r != er || pixel.g != eg || pixel.b != eb) {
        std::println(stderr, "FAIL: {} -- got ({},{},{}) expected ({},{},{})", test_name, pixel.r,
                     pixel.g, pixel.b, er, eg, eb);
        ++failed;
    } else {
        std::println("PASS: {}", test_name);
    }
}

// -------------------------------------------------------------------
// pixel_to_grayscale tests
// -------------------------------------------------------------------

void test_grayscale_black() {
    auto out = pixel_to_grayscale(Pixel{0, 0, 0});
    check_pixel(out, 0, 0, 0, "grayscale black");
}

void test_grayscale_white() {
    auto out = pixel_to_grayscale(Pixel{255, 255, 255});
    check_pixel(out, 255, 255, 255, "grayscale white");
}

void test_grayscale_red() {
    auto out = pixel_to_grayscale(Pixel{255, 0, 0});
    check_pixel(out, 76, 76, 76, "grayscale red -> 76");
}

void test_grayscale_green() {
    auto out = pixel_to_grayscale(Pixel{0, 255, 0});
    check_pixel(out, 150, 150, 150, "grayscale green -> 150");
}

void test_grayscale_mixed() {
    auto out = pixel_to_grayscale(Pixel{100, 150, 200});
    check_pixel(out, 141, 141, 141, "grayscale mixed -> 141");
}

// -------------------------------------------------------------------
// pixel_threshold tests
// -------------------------------------------------------------------

void test_threshold_above() {
    auto out = pixel_threshold(Pixel{255, 255, 255}, 100);
    check_pixel(out, 255, 255, 255, "threshold above (luma 255 > 100) -> white");
}

void test_threshold_below() {
    auto out = pixel_threshold(Pixel{0, 0, 0}, 100);
    check_pixel(out, 0, 0, 0, "threshold below (luma 0 < 100) -> black");
}

void test_threshold_exactly_at() {
    auto out_above = pixel_threshold(Pixel{77, 0, 0}, 23);
    check_pixel(out_above, 255, 255, 255, "threshold at boundary (above) -> white");
    auto out_below = pixel_threshold(Pixel{76, 0, 0}, 23);
    check_pixel(out_below, 0, 0, 0, "threshold at boundary (below) -> black");
}

void test_threshold_edge() {
    auto out = pixel_threshold(Pixel{0, 0, 0}, 0);
    check_pixel(out, 0, 0, 0, "threshold 0 with luma 0 -> black");
}

// -------------------------------------------------------------------
// probe image tests (per-pixel formula checks)
// -------------------------------------------------------------------

void test_grayscale_2x2() {
    check_pixel(pixel_to_grayscale(Pixel{255, 0, 0}), 76, 76, 76, "grayscale 2x2 (255,0,0) -> 76");
    check_pixel(pixel_to_grayscale(Pixel{0, 255, 0}), 150, 150, 150,
                "grayscale 2x2 (0,255,0) -> 150");
    check_pixel(pixel_to_grayscale(Pixel{0, 0, 255}), 29, 29, 29, "grayscale 2x2 (0,0,255) -> 29");
    check_pixel(pixel_to_grayscale(Pixel{30, 31, 66}), 35, 35, 35,
                "grayscale 2x2 (30,31,66) -> 35");
}

void test_threshold_2x2() {
    check_pixel(pixel_threshold(Pixel{255, 0, 0}, 100), 0, 0, 0,
                "threshold 2x2 (255,0,0) T=100 -> black");
    check_pixel(pixel_threshold(Pixel{0, 255, 0}, 100), 255, 255, 255,
                "threshold 2x2 (0,255,0) T=100 -> white");
    check_pixel(pixel_threshold(Pixel{0, 0, 255}, 100), 0, 0, 0,
                "threshold 2x2 (0,0,255) T=100 -> black");
    check_pixel(pixel_threshold(Pixel{30, 31, 66}, 100), 0, 0, 0,
                "threshold 2x2 (30,31,66) T=100 -> black");
}

void test_grayscale_3x3() {
    check_pixel(pixel_to_grayscale(Pixel{30, 31, 45}), 32, 32, 32,
                "grayscale 3x3 (30,31,45) -> 32");
    check_pixel(pixel_to_grayscale(Pixel{30, 31, 66}), 35, 35, 35,
                "grayscale 3x3 (30,31,66) -> 35");
    check_pixel(pixel_to_grayscale(Pixel{128, 128, 128}), 128, 128, 128,
                "grayscale 3x3 (128,128,128) -> 128");
    check_pixel(pixel_to_grayscale(Pixel{100, 100, 100}), 100, 100, 100,
                "grayscale 3x3 (100,100,100) -> 100");
    check_pixel(pixel_to_grayscale(Pixel{30, 164, 196}), 128, 128, 128,
                "grayscale 3x3 (30,164,196) -> 128");
    check_pixel(pixel_to_grayscale(Pixel{30, 164, 200}), 128, 128, 128,
                "grayscale 3x3 (30,164,200) -> 128");
    check_pixel(pixel_to_grayscale(Pixel{10, 20, 30}), 18, 18, 18,
                "grayscale 3x3 (10,20,30) -> 18");
    check_pixel(pixel_to_grayscale(Pixel{200, 100, 50}), 124, 124, 124,
                "grayscale 3x3 (200,100,50) -> 124");
    check_pixel(pixel_to_grayscale(Pixel{50, 200, 100}), 144, 144, 144,
                "grayscale 3x3 (50,200,100) -> 144");
}

void test_threshold_3x3() {
    check_pixel(pixel_threshold(Pixel{30, 31, 45}, 100), 0, 0, 0,
                "threshold 3x3 (30,31,45) T=100 -> black");
    check_pixel(pixel_threshold(Pixel{30, 31, 66}, 100), 0, 0, 0,
                "threshold 3x3 (30,31,66) T=100 -> black");
    check_pixel(pixel_threshold(Pixel{128, 128, 128}, 100), 255, 255, 255,
                "threshold 3x3 (128,128,128) T=100 -> white");
    check_pixel(pixel_threshold(Pixel{100, 100, 100}, 100), 0, 0, 0,
                "threshold 3x3 (100,100,100) T=100 -> black");
    check_pixel(pixel_threshold(Pixel{30, 164, 196}, 100), 255, 255, 255,
                "threshold 3x3 (30,164,196) T=100 -> white");
    check_pixel(pixel_threshold(Pixel{30, 164, 200}, 100), 255, 255, 255,
                "threshold 3x3 (30,164,200) T=100 -> white");
    check_pixel(pixel_threshold(Pixel{10, 20, 30}, 100), 0, 0, 0,
                "threshold 3x3 (10,20,30) T=100 -> black");
    check_pixel(pixel_threshold(Pixel{200, 100, 50}, 100), 255, 255, 255,
                "threshold 3x3 (200,100,50) T=100 -> white");
    check_pixel(pixel_threshold(Pixel{50, 200, 100}, 100), 255, 255, 255,
                "threshold 3x3 (50,200,100) T=100 -> white");
}

// -------------------------------------------------------------------
// parse_filter_args tests
// -------------------------------------------------------------------

void test_parse_no_args() {
    auto r = parse_filter_args(std::to_array<std::string_view>({"prog"}));
    check(!r.has_value(), "no args -> nullopt");
}

void test_parse_grayscale() {
    auto r = parse_filter_args(std::to_array<std::string_view>({"prog", "--grayscale"}));
    check(r.has_value(), "--grayscale -> ok");
    if (r.has_value())
        check(r->args.mode == FilterMode::kGrayscale, "--grayscale mode");
}

void test_parse_grayscale_with_arg() {
    auto r = parse_filter_args(std::to_array<std::string_view>({"prog", "--grayscale", "extra"}));
    check(!r.has_value(), "--grayscale extra -> error");
}

void test_parse_threshold_ok() {
    auto r = parse_filter_args(std::to_array<std::string_view>({"prog", "--threshold", "128"}));
    check(r.has_value(), "--threshold 128 -> ok");
    if (r.has_value()) {
        check(r->args.mode == FilterMode::kThreshold, "--threshold 128 mode");
        check(r->args.threshold == 128, "--threshold 128 value");
    }
}

void test_parse_threshold_edge_low() {
    auto r = parse_filter_args(std::to_array<std::string_view>({"prog", "--threshold", "0"}));
    check(r.has_value(), "--threshold 0 -> ok");
    if (r.has_value())
        check(r->args.threshold == 0, "--threshold 0 value");
}

void test_parse_threshold_edge_high() {
    auto r = parse_filter_args(std::to_array<std::string_view>({"prog", "--threshold", "255"}));
    check(r.has_value(), "--threshold 255 -> ok");
    if (r.has_value())
        check(r->args.threshold == 255, "--threshold 255 value");
}

void test_parse_threshold_no_arg() {
    auto r = parse_filter_args(std::to_array<std::string_view>({"prog", "--threshold"}));
    check(!r.has_value(), "--threshold alone -> error");
}

void test_parse_threshold_not_a_number() {
    auto r = parse_filter_args(std::to_array<std::string_view>({"prog", "--threshold", "abc"}));
    check(!r.has_value(), "--threshold abc -> error");
}

void test_parse_threshold_out_of_range_low() {
    auto r = parse_filter_args(std::to_array<std::string_view>({"prog", "--threshold", "-1"}));
    check(!r.has_value(), "--threshold -1 -> error");
}

void test_parse_threshold_out_of_range_high() {
    auto r = parse_filter_args(std::to_array<std::string_view>({"prog", "--threshold", "256"}));
    check(!r.has_value(), "--threshold 256 -> error");
}

void test_parse_unknown_arg() {
    auto r = parse_filter_args(std::to_array<std::string_view>({"prog", "--blur"}));
    check(!r.has_value(), "--blur -> error");
}

void test_parse_help() {
    auto r = parse_filter_args(std::to_array<std::string_view>({"prog", "--help"}));
    check(r.has_value(), "--help -> ok");
    if (r.has_value())
        check(r->request == FilterRequest::kHelp, "--help kHelp");
}

void test_parse_version() {
    auto r = parse_filter_args(std::to_array<std::string_view>({"prog", "--version"}));
    check(r.has_value(), "--version -> ok");
    if (r.has_value())
        check(r->request == FilterRequest::kVersion, "--version kVersion");
}

void test_parse_run_request() {
    auto r = parse_filter_args(std::to_array<std::string_view>({"prog", "--grayscale"}));
    check(r.has_value(), "--grayscale -> ok");
    if (r.has_value())
        check(r->request == FilterRequest::kRun, "--grayscale kRun");
}

// -------------------------------------------------------------------
// main
// -------------------------------------------------------------------

} // namespace

int main() {
    std::println("--- grayscale tests ---");
    test_grayscale_black();
    test_grayscale_white();
    test_grayscale_red();
    test_grayscale_green();
    test_grayscale_mixed();
    test_grayscale_2x2();
    test_grayscale_3x3();

    std::println("--- threshold tests ---");
    test_threshold_above();
    test_threshold_below();
    test_threshold_exactly_at();
    test_threshold_edge();
    test_threshold_2x2();
    test_threshold_3x3();

    std::println("--- parse_args tests ---");
    test_parse_no_args();
    test_parse_grayscale();
    test_parse_grayscale_with_arg();
    test_parse_threshold_ok();
    test_parse_threshold_edge_low();
    test_parse_threshold_edge_high();
    test_parse_threshold_no_arg();
    test_parse_threshold_not_a_number();
    test_parse_threshold_out_of_range_low();
    test_parse_threshold_out_of_range_high();
    test_parse_unknown_arg();
    test_parse_help();
    test_parse_version();
    test_parse_run_request();

    std::println("---");
    if (failed > 0)
        std::println(stderr, "{} tests FAILED", failed);
    else
        std::println("All tests PASSED");

    return failed > 0 ? 1 : 0;
}
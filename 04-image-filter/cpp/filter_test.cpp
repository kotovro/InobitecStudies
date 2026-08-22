#include "filter.hpp"

#include "../../common/cpp/luma.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <print>
#include <sstream>
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

bool approx_eq(double a, double b, double eps = 1e-9) { return std::fabs(a - b) <= eps; }

// -------------------------------------------------------------------
// luma tests
// -------------------------------------------------------------------

void test_luma_black() { check(approx_eq(luma(0, 0, 0), 0.0), "luma(0,0,0) == 0"); }

void test_luma_white() { check(approx_eq(luma(255, 255, 255), 255.0), "luma(255,255,255) == 255"); }

void test_luma_red() { check(approx_eq(luma(255, 0, 0), 76.245), "luma(255,0,0) ~ 76.245"); }

void test_luma_green() { check(approx_eq(luma(0, 255, 0), 149.685), "luma(0,255,0) ~ 149.685"); }

void test_luma_blue() { check(approx_eq(luma(0, 0, 255), 29.07), "luma(0,0,255) ~ 29.07"); }

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
// run_filter integration tests
// -------------------------------------------------------------------

void test_run_filter_grayscale() {
    auto input_stream = std::istringstream("P3\n2 2\n255\n"
                                           "0 0 0 255 0 0\n"
                                           "0 255 0 0 0 255\n");
    auto output_stream = std::ostringstream();

    int code = run_filter(std::to_array<std::string_view>({"prog", "--grayscale"}), input_stream,
                          output_stream);
    check(code == 0, "run_filter --grayscale -> exit 0");

    auto roundtrip_input = std::istringstream(output_stream.str());
    auto result = Image::read(roundtrip_input);
    check(result.value.has_value(), "run_filter --grayscale: output readable");
    if (result.value.has_value()) {
        const auto& image = *result.value;
        check(image.width() == 2, "run_filter --grayscale: width");
        check(image.height() == 2, "run_filter --grayscale: height");
        check(image.pixel_count() == 4, "run_filter --grayscale: pixel count");
        check_pixel(image.pixels()[0], 0, 0, 0, "run_filter --grayscale: pixel 0");
        check_pixel(image.pixels()[1], 76, 76, 76, "run_filter --grayscale: pixel 1");
        check_pixel(image.pixels()[2], 150, 150, 150, "run_filter --grayscale: pixel 2");
        check_pixel(image.pixels()[3], 29, 29, 29, "run_filter --grayscale: pixel 3");
    }
}

void test_run_filter_threshold() {
    auto input_stream = std::istringstream("P3\n2 1\n255\n"
                                           "0 0 0 128 128 128\n");
    auto output_stream = std::ostringstream();

    int code = run_filter(std::to_array<std::string_view>({"prog", "--threshold", "100"}),
                          input_stream, output_stream);
    check(code == 0, "run_filter --threshold -> exit 0");

    auto roundtrip_input = std::istringstream(output_stream.str());
    auto result = Image::read(roundtrip_input);
    check(result.value.has_value(), "run_filter --threshold: output readable");
    if (result.value.has_value()) {
        const auto& image = *result.value;
        check(image.pixel_count() == 2, "run_filter --threshold: pixel count");
        check_pixel(image.pixels()[0], 0, 0, 0, "run_filter --threshold: pixel 0 black");
        check_pixel(image.pixels()[1], 255, 255, 255, "run_filter --threshold: pixel 1 white");
    }
}

void test_run_filter_empty_input() {
    auto input_stream = std::istringstream("");
    auto output_stream = std::ostringstream();

    int code = run_filter(std::to_array<std::string_view>({"prog", "--grayscale"}), input_stream,
                          output_stream);
    check(code == std::to_underlying(ExitCode::kNoInput), "run_filter empty input -> kNoInput");
}

void test_run_filter_bad_ppm() {
    auto input_stream = std::istringstream("not a ppm");
    auto output_stream = std::ostringstream();

    int code = run_filter(std::to_array<std::string_view>({"prog", "--grayscale"}), input_stream,
                          output_stream);
    check(code == std::to_underlying(ExitCode::kData), "run_filter bad ppm -> kData");
}

void test_run_filter_bad_args() {
    auto input_stream = std::istringstream("P3\n1 1\n255\n0 0 0\n");
    auto output_stream = std::ostringstream();

    int code = run_filter(std::to_array<std::string_view>({"prog", "--invalid"}), input_stream,
                          output_stream);
    check(code == std::to_underlying(ExitCode::kUsage), "run_filter bad args -> kUsage");
}

void test_run_filter_help() {
    auto input_stream = std::istringstream("");
    auto output_stream = std::ostringstream();

    int code = run_filter(std::to_array<std::string_view>({"prog", "--help"}), input_stream,
                          output_stream);
    check(code == std::to_underlying(ExitCode::kOk), "run_filter --help -> exit 0");
    check(output_stream.str().find("Использование:") != std::string::npos,
          "run_filter --help: usage in stdout");
}

void test_run_filter_version() {
    auto input_stream = std::istringstream("");
    auto output_stream = std::ostringstream();

    int code = run_filter(std::to_array<std::string_view>({"prog", "--version"}), input_stream,
                          output_stream);
    check(code == std::to_underlying(ExitCode::kOk), "run_filter --version -> exit 0");
    check(output_stream.str().find("filter") != std::string::npos,
          "run_filter --version: name in stdout");
}

// -------------------------------------------------------------------
// main
// -------------------------------------------------------------------

} // namespace

int main() {
    std::println("--- luma tests ---");
    test_luma_black();
    test_luma_white();
    test_luma_red();
    test_luma_green();
    test_luma_blue();

    std::println("--- grayscale tests ---");
    test_grayscale_black();
    test_grayscale_white();
    test_grayscale_red();
    test_grayscale_green();
    test_grayscale_mixed();

    std::println("--- threshold tests ---");
    test_threshold_above();
    test_threshold_below();
    test_threshold_exactly_at();
    test_threshold_edge();

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

    std::println("--- run_filter integration tests ---");
    test_run_filter_grayscale();
    test_run_filter_threshold();
    test_run_filter_empty_input();
    test_run_filter_bad_ppm();
    test_run_filter_bad_args();
    test_run_filter_help();
    test_run_filter_version();

    std::println("---");
    if (failed > 0)
        std::println(stderr, "{} tests FAILED", failed);
    else
        std::println("All tests PASSED");

    return failed > 0 ? 1 : 0;
}
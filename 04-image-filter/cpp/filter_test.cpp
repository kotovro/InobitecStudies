#include "filter.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <locale>
#include <print>
#include <sstream>
#include <string>

static int failed = 0;

static void check(bool cond, const char* test_name) {
    if (!cond) {
        std::println(stderr, "FAIL: {}", test_name);
        ++failed;
    } else {
        std::println("PASS: {}", test_name);
    }
}

static void check_pixel(const Pixel& pixel, uint8_t er, uint8_t eg, uint8_t eb,
                        const char* test_name) {
    if (pixel.r != er || pixel.g != eg || pixel.b != eb) {
        std::println(stderr, "FAIL: {} -- got ({},{},{}) expected ({},{},{})", test_name, pixel.r,
                     pixel.g, pixel.b, er, eg, eb);
        ++failed;
    } else {
        std::println("PASS: {}", test_name);
    }
}

static bool approx_eq(double a, double b, double eps = 1e-9) { return std::fabs(a - b) <= eps; }

// -------------------------------------------------------------------
// luma tests
// -------------------------------------------------------------------

static void test_luma_black() { check(approx_eq(luma(0, 0, 0), 0.0), "luma(0,0,0) == 0"); }

static void test_luma_white() {
    check(approx_eq(luma(255, 255, 255), 255.0), "luma(255,255,255) == 255");
}

static void test_luma_red() { check(approx_eq(luma(255, 0, 0), 76.245), "luma(255,0,0) ~ 76.245"); }

static void test_luma_green() {
    check(approx_eq(luma(0, 255, 0), 149.685), "luma(0,255,0) ~ 149.685");
}

static void test_luma_blue() { check(approx_eq(luma(0, 0, 255), 29.07), "luma(0,0,255) ~ 29.07"); }

// -------------------------------------------------------------------
// pixel_to_grayscale tests
// -------------------------------------------------------------------

static void test_grayscale_black() {
    auto out = pixel_to_grayscale(Pixel{0, 0, 0});
    check_pixel(out, 0, 0, 0, "grayscale black");
}

static void test_grayscale_white() {
    auto out = pixel_to_grayscale(Pixel{255, 255, 255});
    check_pixel(out, 255, 255, 255, "grayscale white");
}

static void test_grayscale_red() {
    auto out = pixel_to_grayscale(Pixel{255, 0, 0});
    check_pixel(out, 76, 76, 76, "grayscale red -> 76");
}

static void test_grayscale_green() {
    auto out = pixel_to_grayscale(Pixel{0, 255, 0});
    check_pixel(out, 150, 150, 150, "grayscale green -> 150");
}

static void test_grayscale_mixed() {
    auto out = pixel_to_grayscale(Pixel{100, 150, 200});
    check_pixel(out, 141, 141, 141, "grayscale mixed -> 141");
}

// -------------------------------------------------------------------
// pixel_threshold tests
// -------------------------------------------------------------------

static void test_threshold_above() {
    auto out = pixel_threshold(Pixel{255, 255, 255}, 100);
    check_pixel(out, 255, 255, 255, "threshold above (luma 255 > 100) -> white");
}

static void test_threshold_below() {
    auto out = pixel_threshold(Pixel{0, 0, 0}, 100);
    check_pixel(out, 0, 0, 0, "threshold below (luma 0 < 100) -> black");
}

static void test_threshold_exactly_at() {
    auto out_above = pixel_threshold(Pixel{77, 0, 0}, 23);
    check_pixel(out_above, 255, 255, 255, "threshold at boundary (above) -> white");
    auto out_below = pixel_threshold(Pixel{76, 0, 0}, 23);
    check_pixel(out_below, 0, 0, 0, "threshold at boundary (below) -> black");
}

static void test_threshold_edge() {
    auto out = pixel_threshold(Pixel{0, 0, 0}, 0);
    check_pixel(out, 0, 0, 0, "threshold 0 with luma 0 -> black");
}

// -------------------------------------------------------------------
// parse_filter_args tests
// -------------------------------------------------------------------

static void test_parse_no_args() {
    char a0[] = "prog";
    char* argv[] = {a0};
    auto r = parse_filter_args(1, argv);
    check(!r.has_value(), "no args -> nullopt");
}

static void test_parse_grayscale() {
    char a0[] = "prog";
    char a1[] = "--grayscale";
    char* argv[] = {a0, a1};
    auto r = parse_filter_args(2, argv);
    check(r.has_value(), "--grayscale -> ok");
    if (r.has_value())
        check(r->args.mode == FilterMode::kGrayscale, "--grayscale mode");
}

static void test_parse_grayscale_with_arg() {
    char a0[] = "prog";
    char a1[] = "--grayscale";
    char a2[] = "extra";
    char* argv[] = {a0, a1, a2};
    auto r = parse_filter_args(3, argv);
    check(!r.has_value(), "--grayscale extra -> error");
}

static void test_parse_threshold_ok() {
    char a0[] = "prog";
    char a1[] = "--threshold";
    char a2[] = "128";
    char* argv[] = {a0, a1, a2};
    auto r = parse_filter_args(3, argv);
    check(r.has_value(), "--threshold 128 -> ok");
    if (r.has_value()) {
        check(r->args.mode == FilterMode::kThreshold, "--threshold 128 mode");
        check(r->args.threshold == 128, "--threshold 128 value");
    }
}

static void test_parse_threshold_edge_low() {
    char a0[] = "prog";
    char a1[] = "--threshold";
    char a2[] = "0";
    char* argv[] = {a0, a1, a2};
    auto r = parse_filter_args(3, argv);
    check(r.has_value(), "--threshold 0 -> ok");
    if (r.has_value())
        check(r->args.threshold == 0, "--threshold 0 value");
}

static void test_parse_threshold_edge_high() {
    char a0[] = "prog";
    char a1[] = "--threshold";
    char a2[] = "255";
    char* argv[] = {a0, a1, a2};
    auto r = parse_filter_args(3, argv);
    check(r.has_value(), "--threshold 255 -> ok");
    if (r.has_value())
        check(r->args.threshold == 255, "--threshold 255 value");
}

static void test_parse_threshold_no_arg() {
    char a0[] = "prog";
    char a1[] = "--threshold";
    char* argv[] = {a0, a1};
    auto r = parse_filter_args(2, argv);
    check(!r.has_value(), "--threshold alone -> error");
}

static void test_parse_threshold_not_a_number() {
    char a0[] = "prog";
    char a1[] = "--threshold";
    char a2[] = "abc";
    char* argv[] = {a0, a1, a2};
    auto r = parse_filter_args(3, argv);
    check(!r.has_value(), "--threshold abc -> error");
}

static void test_parse_threshold_out_of_range_low() {
    char a0[] = "prog";
    char a1[] = "--threshold";
    char a2[] = "-1";
    char* argv[] = {a0, a1, a2};
    auto r = parse_filter_args(3, argv);
    check(!r.has_value(), "--threshold -1 -> error");
}

static void test_parse_threshold_out_of_range_high() {
    char a0[] = "prog";
    char a1[] = "--threshold";
    char a2[] = "256";
    char* argv[] = {a0, a1, a2};
    auto r = parse_filter_args(3, argv);
    check(!r.has_value(), "--threshold 256 -> error");
}

static void test_parse_unknown_arg() {
    char a0[] = "prog";
    char a1[] = "--blur";
    char* argv[] = {a0, a1};
    auto r = parse_filter_args(2, argv);
    check(!r.has_value(), "--blur -> error");
}

static void test_parse_help() {
    char a0[] = "prog";
    char a1[] = "--help";
    char* argv[] = {a0, a1};
    auto r = parse_filter_args(2, argv);
    check(r.has_value(), "--help -> ok");
    if (r.has_value())
        check(r->request == FilterRequest::kHelp, "--help kHelp");
}

static void test_parse_version() {
    char a0[] = "prog";
    char a1[] = "--version";
    char* argv[] = {a0, a1};
    auto r = parse_filter_args(2, argv);
    check(r.has_value(), "--version -> ok");
    if (r.has_value())
        check(r->request == FilterRequest::kVersion, "--version kVersion");
}

static void test_parse_run_request() {
    char a0[] = "prog";
    char a1[] = "--grayscale";
    char* argv[] = {a0, a1};
    auto r = parse_filter_args(2, argv);
    check(r.has_value(), "--grayscale -> ok");
    if (r.has_value())
        check(r->request == FilterRequest::kRun, "--grayscale kRun");
}

// -------------------------------------------------------------------
// run_filter integration tests
// -------------------------------------------------------------------

static void test_run_filter_grayscale() {
    auto input_stream = std::istringstream("P3\n2 2\n255\n"
                                           "0 0 0 255 0 0\n"
                                           "0 255 0 0 0 255\n");
    auto output_stream = std::ostringstream();
    char a0[] = "prog";
    char a1[] = "--grayscale";
    char* argv[] = {a0, a1};

    int code = run_filter(2, argv, input_stream, output_stream);
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

static void test_run_filter_threshold() {
    auto input_stream = std::istringstream("P3\n2 1\n255\n"
                                           "0 0 0 128 128 128\n");
    auto output_stream = std::ostringstream();
    char a0[] = "prog";
    char a1[] = "--threshold";
    char a2[] = "100";
    char* argv[] = {a0, a1, a2};

    int code = run_filter(3, argv, input_stream, output_stream);
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

static void test_run_filter_empty_input() {
    auto input_stream = std::istringstream("");
    auto output_stream = std::ostringstream();
    char a0[] = "prog";
    char a1[] = "--grayscale";
    char* argv[] = {a0, a1};

    int code = run_filter(2, argv, input_stream, output_stream);
    check(code == static_cast<int>(ExitCode::kNoInput), "run_filter empty input -> kNoInput");
}

static void test_run_filter_bad_ppm() {
    auto input_stream = std::istringstream("not a ppm");
    auto output_stream = std::ostringstream();
    char a0[] = "prog";
    char a1[] = "--grayscale";
    char* argv[] = {a0, a1};

    int code = run_filter(2, argv, input_stream, output_stream);
    check(code == static_cast<int>(ExitCode::kData), "run_filter bad ppm -> kData");
}

static void test_run_filter_bad_args() {
    auto input_stream = std::istringstream("P3\n1 1\n255\n0 0 0\n");
    auto output_stream = std::ostringstream();
    char a0[] = "prog";
    char a1[] = "--invalid";
    char* argv[] = {a0, a1};

    int code = run_filter(2, argv, input_stream, output_stream);
    check(code == static_cast<int>(ExitCode::kUsage), "run_filter bad args -> kUsage");
}

static void test_run_filter_help() {
    auto input_stream = std::istringstream("");
    auto output_stream = std::ostringstream();
    char a0[] = "prog";
    char a1[] = "--help";
    char* argv[] = {a0, a1};

    int code = run_filter(2, argv, input_stream, output_stream);
    check(code == static_cast<int>(ExitCode::kOk), "run_filter --help -> exit 0");
    check(output_stream.str().find("Использование:") != std::string::npos,
          "run_filter --help: usage in stdout");
}

static void test_run_filter_version() {
    auto input_stream = std::istringstream("");
    auto output_stream = std::ostringstream();
    char a0[] = "prog";
    char a1[] = "--version";
    char* argv[] = {a0, a1};

    int code = run_filter(2, argv, input_stream, output_stream);
    check(code == static_cast<int>(ExitCode::kOk), "run_filter --version -> exit 0");
    check(output_stream.str().find("image_filter") != std::string::npos,
          "run_filter --version: name in stdout");
}

// -------------------------------------------------------------------
// main
// -------------------------------------------------------------------

int main() {
    std::setlocale(LC_ALL, "Russian_Russia.1251");

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
#include "hsv_to_rgb.hpp"
#include "parse_args.hpp"
#include "patterns.hpp"

#include <print>
#include <random>

using namespace raster::gen;

namespace {

int failed = 0;

void check(bool cond, std::string_view name, std::string_view detail = {}) {
    if (!cond) {
        std::println(stderr, "FAIL: {} {}", name, detail);
        ++failed;
    } else {
        std::println("PASS: {}", name);
    }
}

// ---- parse_args tests ----

void test_parse_args_no_args() {
    char arg0[] = "prog";
    char* argv[] = {arg0};
    auto r = parse_args(1, argv);
    check(!r.has_value(), "no args");
    if (r.has_value())
        return;
    check(r.error() == ParseError::kNoArg, "no args kNoArg");
}

void test_parse_args_bad_number() {
    char arg0[] = "prog";
    char arg1[] = "abc";
    char* argv[] = {arg0, arg1};
    auto r = parse_args(2, argv);
    check(!r.has_value(), "abc -> error");
    if (r.has_value())
        return;
    check(r.error() == ParseError::kBadNumber, "abc kBadNumber");
}

void test_parse_args_trailing_garbage() {
    char arg0[] = "prog";
    char arg1[] = "4abc";
    char* argv[] = {arg0, arg1};
    auto r = parse_args(2, argv);
    check(!r.has_value(), "4abc - error");
    if (r.has_value())
        return;
    check(r.error() == ParseError::kBadNumber, "4abc kBadNumber");
}

void test_parse_args_negative_number() {
    char arg0[] = "prog";
    char arg1[] = "-5";
    char* argv[] = {arg0, arg1};
    auto r = parse_args(2, argv);
    check(r.has_value(), "-5 - accepted (parse_args only)");
    if (!r.has_value())
        return;
    check(r->args.size == -5, "-5 -> size == -5");
    check(r->args.pattern == Pattern::Gradient, "-5 default Gradient");
}

void test_parse_args_bad_pattern() {
    char arg0[] = "prog";
    char arg1[] = "5";
    char arg2[] = "invalid";
    char* argv[] = {arg0, arg1, arg2};
    auto r = parse_args(3, argv);
    check(!r.has_value(), "invalid pattern error");
    if (r.has_value())
        return;
    check(r.error() == ParseError::kBadPattern, "invalid kBadPattern");
}

void test_parse_args_default_pattern() {
    char arg0[] = "prog";
    char arg1[] = "5";
    char* argv[] = {arg0, arg1};
    auto r = parse_args(2, argv);
    check(r.has_value(), "5 -> ok");
    if (!r.has_value())
        return;
    check(r->args.size == 5, "5 -> size == 5");
    check(r->args.pattern == Pattern::Gradient, "5 default Gradient");
}

void test_parse_args_checker() {
    char arg0[] = "prog";
    char arg1[] = "10";
    char arg2[] = "checker";
    char* argv[] = {arg0, arg1, arg2};
    auto r = parse_args(3, argv);
    check(r.has_value(), "10 checker ok");
    if (!r.has_value())
        return;
    check(r->args.size == 10, "10 checker - size == 10");
    check(r->args.pattern == Pattern::Checker, "10 checker Checker");
}

void test_parse_args_radial() {
    char arg0[] = "prog";
    char arg1[] = "7";
    char arg2[] = "radial";
    char* argv[] = {arg0, arg1, arg2};
    auto r = parse_args(3, argv);
    check(r.has_value(), "7 radial ok");
    if (!r.has_value())
        return;
    check(r->args.size == 7, "7 radial size == 7");
    check(r->args.pattern == Pattern::Radial, "7 radial Radial");
}

// ---- new CLI (--size N [--seed S]) tests ----

void test_new_no_args() {
    char arg0[] = "prog";
    char arg1[] = "--size";
    char* argv[] = {arg0, arg1};
    auto r = parse_args(2, argv);
    check(!r.has_value(), "--size alone -> error");
    if (r.has_value())
        return;
    check(r.error() == ParseError::kNoArg, "--size alone kNoArg");
}

void test_new_bad_size() {
    char arg0[] = "prog";
    char arg1[] = "--size";
    char arg2[] = "abc";
    char* argv[] = {arg0, arg1, arg2};
    auto r = parse_args(3, argv);
    check(!r.has_value(), "--size abc -> error");
    if (r.has_value())
        return;
    check(r.error() == ParseError::kBadNumber, "--size abc kBadNumber");
}

void test_new_ok_no_seed() {
    char arg0[] = "prog";
    char arg1[] = "--size";
    char arg2[] = "10";
    char* argv[] = {arg0, arg1, arg2};
    auto r = parse_args(3, argv);
    check(r.has_value(), "--size 10 ok");
    if (!r.has_value())
        return;
    check(r->args.size == 10, "--size 10 size == 10");
    check(r->args.pattern == Pattern::Random, "--size 10 Random");
    check(!r->args.seed_provided, "--size 10 seed not provided");
}

void test_new_with_seed() {
    char arg0[] = "prog";
    char arg1[] = "--size";
    char arg2[] = "5";
    char arg3[] = "--seed";
    char arg4[] = "42";
    char* argv[] = {arg0, arg1, arg2, arg3, arg4};
    auto r = parse_args(5, argv);
    check(r.has_value(), "--size 5 --seed 42 ok");
    if (!r.has_value())
        return;
    check(r->args.size == 5, "--size 5 size == 5");
    check(r->args.pattern == Pattern::Random, "--size 5 Random");
    check(r->args.seed_provided, "--size 5 seed provided");
    check(r->args.seed == 42, "--size 5 seed == 42");
}

void test_new_bad_seed() {
    char arg0[] = "prog";
    char arg1[] = "--size";
    char arg2[] = "5";
    char arg3[] = "--seed";
    char arg4[] = "abc";
    char* argv[] = {arg0, arg1, arg2, arg3, arg4};
    auto r = parse_args(5, argv);
    check(!r.has_value(), "--size 5 --seed abc -> error");
    if (r.has_value())
        return;
    check(r.error() == ParseError::kBadSeed, "--seed abc kBadSeed");
}

// ---- --help / --version tests ----

void test_help() {
    char arg0[] = "prog";
    char arg1[] = "--help";
    char* argv[] = {arg0, arg1};
    auto r = parse_args(2, argv);
    check(r.has_value(), "--help ok");
    if (!r.has_value())
        return;
    check(r->request == ParseRequest::kHelp, "--help kHelp");
}

void test_version() {
    char arg0[] = "prog";
    char arg1[] = "--version";
    char* argv[] = {arg0, arg1};
    auto r = parse_args(2, argv);
    check(r.has_value(), "--version ok");
    if (!r.has_value())
        return;
    check(r->request == ParseRequest::kVersion, "--version kVersion");
}

void test_run_request() {
    char arg0[] = "prog";
    char arg1[] = "5";
    char* argv[] = {arg0, arg1};
    auto r = parse_args(2, argv);
    check(r.has_value(), "5 ok");
    if (!r.has_value())
        return;
    check(r->request == ParseRequest::kRun, "5 request kRun");
}

// ---- hsv_to_rgb tests ----

void check_rgb(RGB actual, int er, int eg, int eb, std::string_view name) {
    bool ok = (actual.r == er && actual.g == eg && actual.b == eb);
    if (!ok) {
        std::println(stderr, "FAIL: {} got ({},{},{}) expected ({},{},{})", name, actual.r,
                     actual.g, actual.b, er, eg, eb);
        ++failed;
    } else {
        std::println("PASS: {}", name);
    }
}

void test_hsv_red() {
    auto c = hsv_to_rgb(0, 1, 1);
    check_rgb(c, 255, 0, 0, "hsv(0,1,1) == red");
}

void test_hsv_green() {
    auto c = hsv_to_rgb(120, 1, 1);
    check_rgb(c, 0, 255, 0, "hsv(120,1,1) == green");
}

void test_hsv_blue() {
    auto c = hsv_to_rgb(240, 1, 1);
    check_rgb(c, 0, 0, 255, "hsv(240,1,1) == blue");
}

void test_hsv_yellow() {
    auto c = hsv_to_rgb(60, 1, 1);
    check_rgb(c, 255, 255, 0, "hsv(60,1,1) == yellow");
}

void test_hsv_cyan() {
    auto c = hsv_to_rgb(180, 1, 1);
    check_rgb(c, 0, 255, 255, "hsv(180,1,1) == cyan");
}

void test_hsv_magenta() {
    auto c = hsv_to_rgb(300, 1, 1);
    check_rgb(c, 255, 0, 255, "hsv(300,1,1) == magenta");
}

void test_hsv_360_is_red() {
    auto c = hsv_to_rgb(360, 1, 1);
    check_rgb(c, 255, 0, 0, "hsv(360,1,1) == red (wrap)");
}

void test_hsv_zero_saturation() {
    auto c = hsv_to_rgb(0, 0, 1);
    check_rgb(c, 255, 255, 255, "hsv(0,0,1) == white");
}

void test_hsv_zero_value() {
    auto c = hsv_to_rgb(0, 1, 0);
    check_rgb(c, 0, 0, 0, "hsv(0,1,0) == black");
}

void test_hsv_half_saturation() {
    auto c = hsv_to_rgb(0, 0.5, 1);
    check_rgb(c, 255, 127, 127, "hsv(0,0.5,1) == pinkish");
}

// ---- pixel pattern tests ----

void test_gradient_pixel(void) {
    check_rgb(gradient_pixel(0, 0, 3), 0, 255, 0, "gradient (0,0,3)");
    check_rgb(gradient_pixel(1, 0, 3), 127, 255, 0, "gradient (1,0,3)");
    check_rgb(gradient_pixel(2, 0, 3), 255, 255, 0, "gradient (2,0,3)");
    check_rgb(gradient_pixel(0, 1, 3), 0, 127, 0, "gradient (0,1,3)");
    check_rgb(gradient_pixel(1, 1, 3), 127, 127, 0, "gradient (1,1,3)");
    check_rgb(gradient_pixel(2, 1, 3), 255, 127, 0, "gradient (2,1,3)");
    check_rgb(gradient_pixel(0, 2, 3), 0, 0, 0, "gradient (0,2,3)");
    check_rgb(gradient_pixel(2, 2, 3), 255, 0, 0, "gradient (2,2,3)");
    check_rgb(gradient_pixel(0, 0, 1), 0, 255, 0, "gradient size=1 (0,0)");
    check_rgb(gradient_pixel(0, 0, 2), 0, 255, 0, "gradient size=2 (0,0)");
    check_rgb(gradient_pixel(1, 1, 2), 255, 0, 0, "gradient size=2 (1,1)");
    check_rgb(gradient_pixel(0, 0, 512), 0, 255, 0, "gradient size=512 (0,0)");
    check_rgb(gradient_pixel(511, 511, 512), 255, 0, 0, "gradient size=512 (511,511)");
    check_rgb(gradient_pixel(511, 0, 512), 255, 255, 0, "gradient size=512 (511,0)");
    check_rgb(gradient_pixel(0, 511, 512), 0, 0, 0, "gradient size=512 (0,511)");
}

void test_checker_pixel(void) {
    check_rgb(checker_pixel(0, 0), 255, 255, 255, "checker (0,0)");
    check_rgb(checker_pixel(1, 0), 0, 0, 0, "checker (1,0)");
    check_rgb(checker_pixel(0, 1), 0, 0, 0, "checker (0,1)");
    check_rgb(checker_pixel(1, 1), 255, 255, 255, "checker (1,1)");
    check_rgb(checker_pixel(2, 0), 255, 255, 255, "checker (2,0)");
    check_rgb(checker_pixel(2, 1), 0, 0, 0, "checker (2,1)");
    check_rgb(checker_pixel(0, 0), 255, 255, 255, "checker size=1 (0,0)");
    check_rgb(checker_pixel(0, 0), 255, 255, 255, "checker size=512 (0,0)");
    check_rgb(checker_pixel(511, 511), 255, 255, 255, "checker size=512 (511,511)");
    check_rgb(checker_pixel(511, 0), 0, 0, 0, "checker size=512 (511,0)");
    check_rgb(checker_pixel(0, 511), 0, 0, 0, "checker size=512 (0,511)");
}

void test_radial_pixel(void) {
    check_rgb(radial_pixel(0, 0, 3), 255, 0, 0, "radial (0,0,3)");
    check_rgb(radial_pixel(1, 0, 3), 61, 0, 255, "radial (1,0,3)");
    check_rgb(radial_pixel(2, 0, 3), 255, 0, 0, "radial (2,0,3)");
    check_rgb(radial_pixel(0, 1, 3), 61, 0, 255, "radial (0,1,3)");
    check_rgb(radial_pixel(1, 1, 3), 255, 0, 0, "radial (1,1,3)");
    check_rgb(radial_pixel(2, 1, 3), 61, 0, 255, "radial (2,1,3)");
    check_rgb(radial_pixel(0, 2, 3), 255, 0, 0, "radial (0,2,3)");
    check_rgb(radial_pixel(1, 2, 3), 61, 0, 255, "radial (1,2,3)");
    check_rgb(radial_pixel(2, 2, 3), 255, 0, 0, "radial (2,2,3)");
    check_rgb(radial_pixel(0, 0, 1), 255, 0, 0, "radial size=1 (0,0)");
    check_rgb(radial_pixel(0, 0, 512), 255, 0, 0, "radial size=512 corner (0,0)");
    check_rgb(radial_pixel(511, 511, 512), 255, 0, 0, "radial size=512 corner (511,511)");
}

void test_radial_not_degenerate(void) {
    RGB first = radial_pixel(0, 0, 512);
    bool all_same = true;
    for (int32_t y = 0; y < 512 && all_same; ++y)
        for (int32_t x = 0; x < 512 && all_same; ++x) {
            RGB c = radial_pixel(x, y, 512);
            if (c.r != first.r || c.g != first.g || c.b != first.b)
                all_same = false;
        }
    check(!all_same, "radial size=512 not degenerate");
}

// ---- random pixel tests ----

void test_random_not_degenerate(void) {
    std::mt19937 rng(42);
    RGB first = random_pixel(rng);
    bool all_same = true;
    for (int i = 1; i < 1000 && all_same; ++i) {
        RGB c = random_pixel(rng);
        if (c.r != first.r || c.g != first.g || c.b != first.b)
            all_same = false;
    }
    check(!all_same, "random not degenerate (seed=42)");
}

void test_random_zero_seed(void) {
    std::mt19937 rng(0);
    RGB first = random_pixel(rng);
    bool all_same = true;
    for (int i = 1; i < 1000 && all_same; ++i) {
        RGB c = random_pixel(rng);
        if (c.r != first.r || c.g != first.g || c.b != first.b)
            all_same = false;
    }
    check(!all_same, "random not degenerate (seed=0)");
}

void test_random_seed_difference(void) {
    std::mt19937 rng1(42), rng2(43);
    bool differ = false;
    for (int i = 0; i < 10 && !differ; ++i) {
        RGB a = random_pixel(rng1);
        RGB b = random_pixel(rng2);
        if (a.r != b.r || a.g != b.g || a.b != b.b)
            differ = true;
    }
    check(differ, "random different seed differs");
}

void test_random_deterministic(void) {
    std::mt19937 rng1(42), rng2(42);
    bool ok = true;
    for (int i = 0; i < 100 && ok; ++i) {
        RGB a = random_pixel(rng1);
        RGB b = random_pixel(rng2);
        if (a.r != b.r || a.g != b.g || a.b != b.b)
            ok = false;
    }
    check(ok, "random deterministic for same seed");
}

// ---- main ----

} // namespace

int main() {
    std::println("--- parse_args tests ---");
    test_parse_args_no_args();
    test_parse_args_bad_number();
    test_parse_args_trailing_garbage();
    test_parse_args_negative_number();
    test_parse_args_bad_pattern();
    test_parse_args_default_pattern();
    test_parse_args_checker();
    test_parse_args_radial();

    std::println("--- new CLI tests ---");
    test_new_no_args();
    test_new_bad_size();
    test_new_ok_no_seed();
    test_new_with_seed();
    test_new_bad_seed();

    std::println("--- help/version tests ---");
    test_help();
    test_version();
    test_run_request();

    std::println("--- hsv_to_rgb tests ---");
    test_hsv_red();
    test_hsv_green();
    test_hsv_blue();
    test_hsv_yellow();
    test_hsv_cyan();
    test_hsv_magenta();
    test_hsv_360_is_red();
    test_hsv_zero_saturation();
    test_hsv_zero_value();
    test_hsv_half_saturation();

    std::println("--- pixel pattern tests ---");
    test_gradient_pixel();
    test_checker_pixel();
    test_radial_pixel();
    test_radial_not_degenerate();

    std::println("--- random pixel tests ---");
    test_random_not_degenerate();
    test_random_zero_seed();
    test_random_seed_difference();
    test_random_deterministic();

    std::println("---");
    if (failed > 0)
        std::println(stderr, "{} tests FAILED", failed);
    else
        std::println("All tests PASSED");

    return failed > 0 ? 1 : 0;
}
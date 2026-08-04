#include "hsv_to_rgb.hpp"
#include "parse_args.hpp"

#include <print>

static int failed = 0;

static void check(bool cond, std::string_view name, std::string_view detail = {}) {
    if (!cond) {
        std::println(stderr, "FAIL: {} {}", name, detail);
        ++failed;
    } else {
        std::println("PASS: {}", name);
    }
}

// ---- parse_args tests ----

static void test_parse_args_no_args() {
    char arg0[] = "prog";
    char* argv[] = {arg0};
    auto r = parse_args(1, argv);
    check(!r.has_value(), "no args");
    if (r.has_value())
        return;
    check(r.error() == ParseError::kNoArg, "no args kNoArg");
}

static void test_parse_args_bad_number() {
    char arg0[] = "prog";
    char arg1[] = "abc";
    char* argv[] = {arg0, arg1};
    auto r = parse_args(2, argv);
    check(!r.has_value(), "abc -> error");
    if (r.has_value())
        return;
    check(r.error() == ParseError::kBadNumber, "abc kBadNumber");
}

static void test_parse_args_trailing_garbage() {
    char arg0[] = "prog";
    char arg1[] = "4abc";
    char* argv[] = {arg0, arg1};
    auto r = parse_args(2, argv);
    check(!r.has_value(), "4abc - error");
    if (r.has_value())
        return;
    check(r.error() == ParseError::kBadNumber, "4abc kBadNumber");
}

static void test_parse_args_negative_number() {
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

static void test_parse_args_bad_pattern() {
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

static void test_parse_args_default_pattern() {
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

static void test_parse_args_checker() {
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

static void test_parse_args_radial() {
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

static void test_new_no_args() {
    char arg0[] = "prog";
    char arg1[] = "--size";
    char* argv[] = {arg0, arg1};
    auto r = parse_args(2, argv);
    check(!r.has_value(), "--size alone -> error");
    if (r.has_value())
        return;
    check(r.error() == ParseError::kNoArg, "--size alone kNoArg");
}

static void test_new_bad_size() {
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

static void test_new_ok_no_seed() {
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

static void test_new_with_seed() {
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

static void test_new_bad_seed() {
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

static void test_help() {
    char arg0[] = "prog";
    char arg1[] = "--help";
    char* argv[] = {arg0, arg1};
    auto r = parse_args(2, argv);
    check(r.has_value(), "--help ok");
    if (!r.has_value())
        return;
    check(r->request == ParseRequest::kHelp, "--help kHelp");
}

static void test_version() {
    char arg0[] = "prog";
    char arg1[] = "--version";
    char* argv[] = {arg0, arg1};
    auto r = parse_args(2, argv);
    check(r.has_value(), "--version ok");
    if (!r.has_value())
        return;
    check(r->request == ParseRequest::kVersion, "--version kVersion");
}

static void test_run_request() {
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

static void check_rgb(RGB actual, int er, int eg, int eb, std::string_view name) {
    bool ok = (actual.r == er && actual.g == eg && actual.b == eb);
    if (!ok) {
        std::println(stderr, "FAIL: {} got ({},{},{}) expected ({},{},{})", name, actual.r,
                     actual.g, actual.b, er, eg, eb);
        ++failed;
    } else {
        std::println("PASS: {}", name);
    }
}

static void test_hsv_red() {
    auto c = hsv_to_rgb(0, 1, 1);
    check_rgb(c, 255, 0, 0, "hsv(0,1,1) == red");
}

static void test_hsv_green() {
    auto c = hsv_to_rgb(120, 1, 1);
    check_rgb(c, 0, 255, 0, "hsv(120,1,1) == green");
}

static void test_hsv_blue() {
    auto c = hsv_to_rgb(240, 1, 1);
    check_rgb(c, 0, 0, 255, "hsv(240,1,1) == blue");
}

static void test_hsv_yellow() {
    auto c = hsv_to_rgb(60, 1, 1);
    check_rgb(c, 255, 255, 0, "hsv(60,1,1) == yellow");
}

static void test_hsv_cyan() {
    auto c = hsv_to_rgb(180, 1, 1);
    check_rgb(c, 0, 255, 255, "hsv(180,1,1) == cyan");
}

static void test_hsv_magenta() {
    auto c = hsv_to_rgb(300, 1, 1);
    check_rgb(c, 255, 0, 255, "hsv(300,1,1) == magenta");
}

static void test_hsv_360_is_red() {
    auto c = hsv_to_rgb(360, 1, 1);
    check_rgb(c, 255, 0, 0, "hsv(360,1,1) == red (wrap)");
}

static void test_hsv_zero_saturation() {
    auto c = hsv_to_rgb(0, 0, 1);
    check_rgb(c, 255, 255, 255, "hsv(0,0,1) == white");
}

static void test_hsv_zero_value() {
    auto c = hsv_to_rgb(0, 1, 0);
    check_rgb(c, 0, 0, 0, "hsv(0,1,0) == black");
}

static void test_hsv_half_saturation() {
    auto c = hsv_to_rgb(0, 0.5, 1);
    check_rgb(c, 255, 127, 127, "hsv(0,0.5,1) == pinkish");
}

// ---- main ----

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

    std::println("---");
    if (failed > 0)
        std::println(stderr, "{} tests FAILED", failed);
    else
        std::println("All tests PASSED");

    return failed > 0 ? 1 : 0;
}

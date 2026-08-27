#include "hsv_to_rgb.h"
#include "parse_args.h"
#include "patterns.h"

#include <stdio.h>
#include <string.h>

static int failed = 0;

static void check(int cond, const char* name) {
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", name);
        ++failed;
    } else {
        printf("PASS: %s\n", name);
    }
}

// ---- parse_args tests ----

static void test_no_args(void) {
    char* argv[] = {"prog"};
    struct ParseResult r = parse_args(1, argv);
    check(r.error == PE_NOARG, "no args -> PE_NOARG");
}

static void test_bad_number(void) {
    char* argv[] = {"prog", "abc"};
    struct ParseResult r = parse_args(2, argv);
    check(r.error == PE_BADNUMBER, "abc -> PE_BADNUMBER");
}

static void test_trailing_garbage(void) {
    char* argv[] = {"prog", "4abc"};
    struct ParseResult r = parse_args(2, argv);
    check(r.error == PE_BADNUMBER, "4abc -> PE_BADNUMBER");
}

static void test_negative_number(void) {
    char* argv[] = {"prog", "-5"};
    struct ParseResult r = parse_args(2, argv);
    check(r.error == PE_OK, "-5 -> accepted (parse_args only)");
    if (r.error == PE_OK) {
        check(r.size == -5, "-5 -> size == -5");
        check(r.pattern == PATTERN_GRADIENT, "-5 -> default Gradient");
    }
}

static void test_bad_pattern(void) {
    char* argv[] = {"prog", "5", "invalid"};
    struct ParseResult r = parse_args(3, argv);
    check(r.error == PE_BADPATTERN, "invalid pattern -> PE_BADPATTERN");
}

static void test_default_pattern(void) {
    char* argv[] = {"prog", "5"};
    struct ParseResult r = parse_args(2, argv);
    check(r.error == PE_OK, "5 -> ok");
    if (r.error == PE_OK) {
        check(r.size == 5, "5 -> size == 5");
        check(r.pattern == PATTERN_GRADIENT, "5 -> default Gradient");
    }
}

static void test_checker_pattern(void) {
    char* argv[] = {"prog", "10", "checker"};
    struct ParseResult r = parse_args(3, argv);
    check(r.error == PE_OK, "10 checker -> ok");
    if (r.error == PE_OK) {
        check(r.size == 10, "10 checker -> size == 10");
        check(r.pattern == PATTERN_CHECKER, "10 checker -> Checker");
    }
}

static void test_radial_pattern(void) {
    char* argv[] = {"prog", "7", "radial"};
    struct ParseResult r = parse_args(3, argv);
    check(r.error == PE_OK, "7 radial -> ok");
    if (r.error == PE_OK) {
        check(r.size == 7, "7 radial -> size == 7");
        check(r.pattern == PATTERN_RADIAL, "7 radial -> Radial");
    }
}

// ---- new CLI (--size N [--seed S]) tests ----

static void test_new_no_args(void) {
    char* argv[] = {"prog", "--size"};
    struct ParseResult r = parse_args(2, argv);
    check(r.error == PE_NOARG, "--size alone -> PE_NOARG");
}

static void test_new_bad_size(void) {
    char* argv[] = {"prog", "--size", "abc"};
    struct ParseResult r = parse_args(3, argv);
    check(r.error == PE_BADNUMBER, "--size abc -> PE_BADNUMBER");
}

static void test_new_ok_no_seed(void) {
    char* argv[] = {"prog", "--size", "10"};
    struct ParseResult r = parse_args(3, argv);
    check(r.error == PE_OK, "--size 10 -> PE_OK");
    if (r.error == PE_OK) {
        check(r.size == 10, "--size 10 -> size == 10");
        check(r.pattern == PATTERN_RANDOM, "--size 10 -> random");
        check(r.seed_provided == 0, "--size 10 -> seed not provided");
    }
}

static void test_new_with_seed(void) {
    char* argv[] = {"prog", "--size", "5", "--seed", "42"};
    struct ParseResult r = parse_args(5, argv);
    check(r.error == PE_OK, "--size 5 --seed 42 -> PE_OK");
    if (r.error == PE_OK) {
        check(r.size == 5, "--size 5 --seed 42 -> size == 5");
        check(r.pattern == PATTERN_RANDOM, "--size 5 --seed 42 -> random");
        check(r.seed_provided == 1, "--size 5 --seed 42 -> seed provided");
        check(r.seed == 42, "--size 5 --seed 42 -> seed == 42");
    }
}

static void test_new_bad_seed(void) {
    char* argv[] = {"prog", "--size", "5", "--seed", "abc"};
    struct ParseResult r = parse_args(5, argv);
    check(r.error == PE_BADSEED, "--seed abc -> PE_BADSEED");
}

static void test_new_negative_seed(void) {
    char* argv[] = {"prog", "--size", "5", "--seed", "-5"};
    struct ParseResult r = parse_args(5, argv);
    check(r.error == PE_BADSEED, "--seed -5 -> PE_BADSEED");
}

// ---- --help / --version tests ----

static void test_help(void) {
    char* argv[] = {"prog", "--help"};
    struct ParseResult r = parse_args(2, argv);
    check(r.error == PE_OK && r.request == PR_HELP, "--help -> PR_HELP");
}

static void test_version(void) {
    char* argv[] = {"prog", "--version"};
    struct ParseResult r = parse_args(2, argv);
    check(r.error == PE_OK && r.request == PR_VERSION, "--version -> PR_VERSION");
}

static void test_run_request(void) {
    char* argv[] = {"prog", "5"};
    struct ParseResult r = parse_args(2, argv);
    check(r.error == PE_OK && r.request == PR_RUN, "5 -> PR_RUN");
}

// ---- hsv_to_rgb tests ----

static void check_rgb(struct RGB actual, int er, int eg, int eb, const char* name) {
    int ok = (actual.r == er && actual.g == eg && actual.b == eb);
    if (!ok)
        fprintf(stderr, "FAIL: %s -- got (%d,%d,%d) expected (%d,%d,%d)\n", name, actual.r,
                actual.g, actual.b, er, eg, eb);
    else
        printf("PASS: %s\n", name);
    if (!ok)
        ++failed;
}

static void test_hsv_red(void) { check_rgb(hsv_to_rgb(0, 1, 1), 255, 0, 0, "hsv(0,1,1) == red"); }

static void test_hsv_green(void) {
    check_rgb(hsv_to_rgb(120, 1, 1), 0, 255, 0, "hsv(120,1,1) == green");
}

static void test_hsv_blue(void) {
    check_rgb(hsv_to_rgb(240, 1, 1), 0, 0, 255, "hsv(240,1,1) == blue");
}

static void test_hsv_yellow(void) {
    check_rgb(hsv_to_rgb(60, 1, 1), 255, 255, 0, "hsv(60,1,1) == yellow");
}

static void test_hsv_cyan(void) {
    check_rgb(hsv_to_rgb(180, 1, 1), 0, 255, 255, "hsv(180,1,1) == cyan");
}

static void test_hsv_magenta(void) {
    check_rgb(hsv_to_rgb(300, 1, 1), 255, 0, 255, "hsv(300,1,1) == magenta");
}

static void test_hsv_360_is_red(void) {
    check_rgb(hsv_to_rgb(360, 1, 1), 255, 0, 0, "hsv(360,1,1) == red (wrap)");
}

static void test_hsv_zero_saturation(void) {
    check_rgb(hsv_to_rgb(0, 0, 1), 255, 255, 255, "hsv(0,0,1) == white");
}

static void test_hsv_zero_value(void) {
    check_rgb(hsv_to_rgb(0, 1, 0), 0, 0, 0, "hsv(0,1,0) == black");
}

static void test_hsv_half_saturation(void) {
    struct RGB c = hsv_to_rgb(0, 0.5, 1);
    check_rgb(c, 255, 127, 127, "hsv(0,0.5,1) == pinkish");
}

static void test_gradient_pixel(void) {
    check_rgb(gradient_pixel(0, 0, 3), 0, 255, 0, "gradient (0,0,3)");
    check_rgb(gradient_pixel(1, 0, 3), 127, 255, 0, "gradient (1,0,3)");
    check_rgb(gradient_pixel(2, 0, 3), 255, 255, 0, "gradient (2,0,3)");
    check_rgb(gradient_pixel(0, 1, 3), 0, 127, 0, "gradient (0,1,3)");
    check_rgb(gradient_pixel(1, 1, 3), 127, 127, 0, "gradient (1,1,3)");
    check_rgb(gradient_pixel(2, 1, 3), 255, 127, 0, "gradient (2,1,3)");
    check_rgb(gradient_pixel(0, 2, 3), 0, 0, 0, "gradient (0,2,3)");
    check_rgb(gradient_pixel(1, 2, 3), 127, 0, 0, "gradient (1,2,3)");
    check_rgb(gradient_pixel(2, 2, 3), 255, 0, 0, "gradient (2,2,3)");
    check_rgb(gradient_pixel(0, 0, 1), 0, 255, 0, "gradient size=1 (0,0)");
    check_rgb(gradient_pixel(0, 0, 2), 0, 255, 0, "gradient size=2 (0,0)");
    check_rgb(gradient_pixel(1, 1, 2), 255, 0, 0, "gradient size=2 (1,1)");
}

static void test_checker_pixel(void) {
    check_rgb(checker_pixel(0, 0), 255, 255, 255, "checker (0,0)");
    check_rgb(checker_pixel(1, 0), 0, 0, 0, "checker (1,0)");
    check_rgb(checker_pixel(0, 1), 0, 0, 0, "checker (0,1)");
    check_rgb(checker_pixel(1, 1), 255, 255, 255, "checker (1,1)");
    check_rgb(checker_pixel(2, 0), 255, 255, 255, "checker (2,0)");
    check_rgb(checker_pixel(2, 1), 0, 0, 0, "checker (2,1)");
    check_rgb(checker_pixel(2, 2), 255, 255, 255, "checker (2,2)");
    check_rgb(checker_pixel(0, 0), 255, 255, 255, "checker size=1 (0,0)");
}

static void test_radial_pixel(void) {
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
}

// ---- random pixel tests ----

static void test_random_deterministic(void) {
    uint32_t s1 = 42, s2 = 42;
    int ok = 1;
    for (int i = 0; i < 100; ++i) {
        struct RGB a = random_pixel(&s1);
        struct RGB b = random_pixel(&s2);
        if (a.r != b.r || a.g != b.g || a.b != b.b) {
            ok = 0;
            break;
        }
    }
    check(ok, "random deterministic for same seed");
}

static void test_random_seed_difference(void) {
    uint32_t s1 = 42, s2 = 43;
    struct RGB a = random_pixel(&s1);
    struct RGB b = random_pixel(&s2);
    check(a.r != b.r || a.g != b.g || a.b != b.b, "random different seed differs");
}

static void test_random_not_degenerate(void) {
    uint32_t state = 42;
    struct RGB first = random_pixel(&state);
    int all_same = 1;
    for (int i = 1; i < 1000; ++i) {
        struct RGB c = random_pixel(&state);
        if (c.r != first.r || c.g != first.g || c.b != first.b) {
            all_same = 0;
            break;
        }
    }
    check(!all_same, "random not degenerate (not all pixels equal)");
}

static void test_random_zero_state_protection(void) {
    uint32_t s_zero = 0, s_42 = 42;
    struct RGB a = random_pixel(&s_zero);
    struct RGB b = random_pixel(&s_42);
    check_rgb(a, b.r, b.g, b.b, "random *state==0 -> 42");
}

// ---- main ----

int main(void) {
    printf("--- parse_args tests ---\n");
    test_no_args();
    test_bad_number();
    test_trailing_garbage();
    test_negative_number();
    test_bad_pattern();
    test_default_pattern();
    test_checker_pattern();
    test_radial_pattern();

    printf("--- new CLI tests ---\n");
    test_new_no_args();
    test_new_bad_size();
    test_new_ok_no_seed();
    test_new_with_seed();
    test_new_bad_seed();
    test_new_negative_seed();

    printf("--- help/version tests ---\n");
    test_help();
    test_version();
    test_run_request();

    printf("--- hsv_to_rgb tests ---\n");
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

    printf("--- pixel pattern tests ---\n");
    test_gradient_pixel();
    test_checker_pixel();
    test_radial_pixel();

    printf("--- random pixel tests ---\n");
    test_random_deterministic();
    test_random_seed_difference();
    test_random_not_degenerate();
    test_random_zero_state_protection();

    printf("---\n");
    if (failed > 0)
        fprintf(stderr, "%d tests FAILED\n", failed);
    else
        printf("All tests PASSED\n");

    return failed > 0 ? 1 : 0;
}
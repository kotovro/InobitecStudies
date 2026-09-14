#include "filter.h"

#include <stdint.h>
#include <stdio.h>

static int failed = 0;

static void check(int cond, const char* test_name) {
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", test_name);
        ++failed;
    } else {
        printf("PASS: %s\n", test_name);
    }
}

static void check_pixel(const struct Pixel* pixel, uint8_t er, uint8_t eg, uint8_t eb,
                        const char* test_name) {
    if (pixel->r != er || pixel->g != eg || pixel->b != eb) {
        fprintf(stderr, "FAIL: %s -- got (%d,%d,%d) expected (%d,%d,%d)\n", test_name, pixel->r,
                pixel->g, pixel->b, er, eg, eb);
        ++failed;
    } else {
        printf("PASS: %s\n", test_name);
    }
}

// -------------------------------------------------------------------
// pixel_to_grayscale tests
// -------------------------------------------------------------------

static void test_grayscale_black(void) {
    struct Pixel src = {0, 0, 0}, dst;
    pixel_to_grayscale(&src, &dst);
    check_pixel(&dst, 0, 0, 0, "grayscale black");
}

static void test_grayscale_red(void) {
    struct Pixel src = {255, 0, 0}, dst;
    pixel_to_grayscale(&src, &dst);
    check_pixel(&dst, 76, 76, 76, "grayscale red -> 76");
}

static void test_grayscale_mixed(void) {
    struct Pixel src = {100, 150, 200}, dst;
    pixel_to_grayscale(&src, &dst);
    // luma = 140.75 -> 141
    check_pixel(&dst, 141, 141, 141, "grayscale mixed -> 141");
}

// -------------------------------------------------------------------
// pixel_threshold tests
// -------------------------------------------------------------------

static void test_threshold_above(void) {
    struct Pixel src = {255, 255, 255}, dst;
    pixel_threshold(&src, 100, &dst);
    check_pixel(&dst, 255, 255, 255, "threshold above -> white");
}

static void test_threshold_below(void) {
    struct Pixel src = {0, 0, 0}, dst;
    pixel_threshold(&src, 100, &dst);
    check_pixel(&dst, 0, 0, 0, "threshold below -> black");
}

static void test_threshold_boundary(void) {
    // luma(76,0,0) = 22.724, < 23
    // luma(77,0,0) = 23.023, > 23
    struct Pixel src_low = {76, 0, 0}, src_high = {77, 0, 0};
    struct Pixel dst;
    pixel_threshold(&src_low, 23, &dst);
    check_pixel(&dst, 0, 0, 0, "threshold boundary low -> black");
    pixel_threshold(&src_high, 23, &dst);
    check_pixel(&dst, 255, 255, 255, "threshold boundary high -> white");
}

// -------------------------------------------------------------------
// probe image tests (per-pixel formula checks)
// -------------------------------------------------------------------

static void test_grayscale_2x2(void) {
    struct Pixel src, dst;

    src = (struct Pixel){255, 0, 0};
    pixel_to_grayscale(&src, &dst);
    check_pixel(&dst, 76, 76, 76, "grayscale 2x2 (255,0,0) -> 76");

    src = (struct Pixel){0, 255, 0};
    pixel_to_grayscale(&src, &dst);
    check_pixel(&dst, 150, 150, 150, "grayscale 2x2 (0,255,0) -> 150");

    src = (struct Pixel){0, 0, 255};
    pixel_to_grayscale(&src, &dst);
    check_pixel(&dst, 29, 29, 29, "grayscale 2x2 (0,0,255) -> 29");

    src = (struct Pixel){30, 31, 66};
    pixel_to_grayscale(&src, &dst);
    check_pixel(&dst, 35, 35, 35, "grayscale 2x2 (30,31,66) -> 35");
}

static void test_threshold_2x2(void) {
    struct Pixel src, dst;

    src = (struct Pixel){255, 0, 0};
    pixel_threshold(&src, 100, &dst);
    check_pixel(&dst, 0, 0, 0, "threshold 2x2 (255,0,0) T=100 -> black");

    src = (struct Pixel){0, 255, 0};
    pixel_threshold(&src, 100, &dst);
    check_pixel(&dst, 255, 255, 255, "threshold 2x2 (0,255,0) T=100 -> white");

    src = (struct Pixel){0, 0, 255};
    pixel_threshold(&src, 100, &dst);
    check_pixel(&dst, 0, 0, 0, "threshold 2x2 (0,0,255) T=100 -> black");

    src = (struct Pixel){30, 31, 66};
    pixel_threshold(&src, 100, &dst);
    check_pixel(&dst, 0, 0, 0, "threshold 2x2 (30,31,66) T=100 -> black");
}

static void test_grayscale_3x3(void) {
    struct Pixel src, dst;

    src = (struct Pixel){30, 31, 45};
    pixel_to_grayscale(&src, &dst);
    check_pixel(&dst, 32, 32, 32, "grayscale 3x3 (30,31,45) -> 32");

    src = (struct Pixel){30, 31, 66};
    pixel_to_grayscale(&src, &dst);
    check_pixel(&dst, 35, 35, 35, "grayscale 3x3 (30,31,66) -> 35");

    src = (struct Pixel){128, 128, 128};
    pixel_to_grayscale(&src, &dst);
    check_pixel(&dst, 128, 128, 128, "grayscale 3x3 (128,128,128) -> 128");

    src = (struct Pixel){100, 100, 100};
    pixel_to_grayscale(&src, &dst);
    check_pixel(&dst, 100, 100, 100, "grayscale 3x3 (100,100,100) -> 100");

    src = (struct Pixel){30, 164, 196};
    pixel_to_grayscale(&src, &dst);
    check_pixel(&dst, 128, 128, 128, "grayscale 3x3 (30,164,196) -> 128");

    src = (struct Pixel){30, 164, 200};
    pixel_to_grayscale(&src, &dst);
    check_pixel(&dst, 128, 128, 128, "grayscale 3x3 (30,164,200) -> 128");

    src = (struct Pixel){10, 20, 30};
    pixel_to_grayscale(&src, &dst);
    check_pixel(&dst, 18, 18, 18, "grayscale 3x3 (10,20,30) -> 18");

    src = (struct Pixel){200, 100, 50};
    pixel_to_grayscale(&src, &dst);
    check_pixel(&dst, 124, 124, 124, "grayscale 3x3 (200,100,50) -> 124");

    src = (struct Pixel){50, 200, 100};
    pixel_to_grayscale(&src, &dst);
    check_pixel(&dst, 144, 144, 144, "grayscale 3x3 (50,200,100) -> 144");
}

static void test_threshold_3x3(void) {
    struct Pixel src, dst;

    src = (struct Pixel){30, 31, 45};
    pixel_threshold(&src, 100, &dst);
    check_pixel(&dst, 0, 0, 0, "threshold 3x3 (30,31,45) T=100 -> black");

    src = (struct Pixel){30, 31, 66};
    pixel_threshold(&src, 100, &dst);
    check_pixel(&dst, 0, 0, 0, "threshold 3x3 (30,31,66) T=100 -> black");

    src = (struct Pixel){128, 128, 128};
    pixel_threshold(&src, 100, &dst);
    check_pixel(&dst, 255, 255, 255, "threshold 3x3 (128,128,128) T=100 -> white");

    src = (struct Pixel){100, 100, 100};
    pixel_threshold(&src, 100, &dst);
    check_pixel(&dst, 0, 0, 0, "threshold 3x3 (100,100,100) T=100 -> black");

    src = (struct Pixel){30, 164, 196};
    pixel_threshold(&src, 100, &dst);
    check_pixel(&dst, 255, 255, 255, "threshold 3x3 (30,164,196) T=100 -> white");

    src = (struct Pixel){30, 164, 200};
    pixel_threshold(&src, 100, &dst);
    check_pixel(&dst, 255, 255, 255, "threshold 3x3 (30,164,200) T=100 -> white");

    src = (struct Pixel){10, 20, 30};
    pixel_threshold(&src, 100, &dst);
    check_pixel(&dst, 0, 0, 0, "threshold 3x3 (10,20,30) T=100 -> black");

    src = (struct Pixel){200, 100, 50};
    pixel_threshold(&src, 100, &dst);
    check_pixel(&dst, 255, 255, 255, "threshold 3x3 (200,100,50) T=100 -> white");

    src = (struct Pixel){50, 200, 100};
    pixel_threshold(&src, 100, &dst);
    check_pixel(&dst, 255, 255, 255, "threshold 3x3 (50,200,100) T=100 -> white");
}

// -------------------------------------------------------------------
// parse_filter_args tests
// -------------------------------------------------------------------

static void test_parse_grayscale(void) {
    char* argv[] = {"prog", "--grayscale"};
    struct FilterParseResult res;
    check(parse_filter_args(2, argv, &res) == 0, "--grayscale -> ok");
    if (parse_filter_args(2, argv, &res) == 0) {
        check(res.request == FILTER_RUN, "--grayscale request RUN");
        check(res.args.mode == FILTER_GRAYSCALE, "--grayscale mode");
    }
}

static void test_parse_grayscale_with_arg(void) {
    char* argv[] = {"prog", "--grayscale", "extra"};
    struct FilterParseResult res;
    check(parse_filter_args(3, argv, &res) != 0, "--grayscale extra -> error");
}

static void test_parse_threshold_ok(void) {
    char* argv[] = {"prog", "--threshold", "128"};
    struct FilterParseResult res;
    check(parse_filter_args(3, argv, &res) == 0, "--threshold 128 -> ok");
    if (parse_filter_args(3, argv, &res) == 0) {
        check(res.args.mode == FILTER_THRESHOLD, "--threshold 128 mode");
        check(res.args.threshold == 128, "--threshold 128 value");
    }
}

static void test_parse_threshold_no_arg(void) {
    char* argv[] = {"prog", "--threshold"};
    struct FilterParseResult res;
    check(parse_filter_args(2, argv, &res) != 0, "--threshold alone -> error");
}

static void test_parse_threshold_bad_value(void) {
    char* argv[] = {"prog", "--threshold", "abc"};
    struct FilterParseResult res;
    check(parse_filter_args(3, argv, &res) != 0, "--threshold abc -> error");
}

static void test_parse_threshold_out_of_range(void) {
    char* argv[] = {"prog", "--threshold", "256"};
    struct FilterParseResult res;
    check(parse_filter_args(3, argv, &res) != 0, "--threshold 256 -> error");
}

static void test_parse_help(void) {
    char* argv[] = {"prog", "--help"};
    struct FilterParseResult res;
    check(parse_filter_args(2, argv, &res) == 0, "--help -> ok");
    if (parse_filter_args(2, argv, &res) == 0)
        check(res.request == FILTER_HELP, "--help request HELP");
}

static void test_parse_version(void) {
    char* argv[] = {"prog", "--version"};
    struct FilterParseResult res;
    check(parse_filter_args(2, argv, &res) == 0, "--version -> ok");
    if (parse_filter_args(2, argv, &res) == 0)
        check(res.request == FILTER_VERSION, "--version request VERSION");
}

static void test_parse_unknown(void) {
    char* argv[] = {"prog", "--blur"};
    struct FilterParseResult res;
    check(parse_filter_args(2, argv, &res) != 0, "--blur -> error");
}

// -------------------------------------------------------------------
// main
// -------------------------------------------------------------------

int main(void) {
    printf("--- grayscale tests (C) ---\n");
    test_grayscale_black();
    test_grayscale_red();
    test_grayscale_mixed();
    test_grayscale_2x2();
    test_grayscale_3x3();

    printf("--- threshold tests (C) ---\n");
    test_threshold_above();
    test_threshold_below();
    test_threshold_boundary();
    test_threshold_2x2();
    test_threshold_3x3();

    printf("--- parse_args tests (C) ---\n");
    test_parse_grayscale();
    test_parse_grayscale_with_arg();
    test_parse_threshold_ok();
    test_parse_threshold_no_arg();
    test_parse_threshold_bad_value();
    test_parse_threshold_out_of_range();
    test_parse_unknown();
    test_parse_help();
    test_parse_version();

    printf("---\n");
    if (failed > 0)
        fprintf(stderr, "%d tests FAILED\n", failed);
    else
        printf("All tests PASSED\n");

    return failed > 0 ? 1 : 0;
}
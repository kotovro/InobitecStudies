#include "filter.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../common/c/luma.h"

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

#define LUMA_EPS 1e-9
static int approx_eq(double a, double b) { return fabs(a - b) <= LUMA_EPS; }

static FILE* make_ppm(const char* data) {
    FILE* f = tmpfile();
    if (!f) {
        fprintf(stderr, "FAIL: tmpfile creation failed\n");
        return NULL;
    }
    fputs(data, f);
    rewind(f);
    return f;
}

// -------------------------------------------------------------------
// luma tests
// -------------------------------------------------------------------

static void test_luma_black(void) { check(approx_eq(luma(0, 0, 0), 0.0), "luma(0,0,0) == 0"); }

static void test_luma_red(void) {
    check(approx_eq(luma(255, 0, 0), 76.245), "luma(255,0,0) ~ 76.245");
}

static void test_luma_green(void) {
    check(approx_eq(luma(0, 255, 0), 149.685), "luma(0,255,0) ~ 149.685");
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
// run_filter integration tests
// -------------------------------------------------------------------

static void test_run_filter_grayscale(void) {
    FILE* input = make_ppm("P3\n2 2\n255\n"
                           "0 0 0 255 0 0\n"
                           "0 255 0 0 0 255\n");
    FILE* output = tmpfile();
    char* argv[] = {"prog", "--grayscale"};
    int code = run_filter(2, argv, input, output);

    check(code == EC_OK, "run_filter --grayscale -> exit 0");

    rewind(output);
    struct PpmResult result = ppm_read(output);
    check(result.error == PRE_OK, "run_filter --grayscale: output readable");
    if (result.error == PRE_OK) {
        check(result.image.width == 2, "run_filter --grayscale: width");
        check(result.image.height == 2, "run_filter --grayscale: height");
        check_pixel(&result.image.pixels[0], 0, 0, 0, "run_filter --grayscale: pixel 0");
        check_pixel(&result.image.pixels[1], 76, 76, 76, "run_filter --grayscale: pixel 1");
        check_pixel(&result.image.pixels[2], 150, 150, 150, "run_filter --grayscale: pixel 2");
        check_pixel(&result.image.pixels[3], 29, 29, 29, "run_filter --grayscale: pixel 3");
        ppm_image_free(&result.image);
    }

    fclose(input);
    fclose(output);
}

static void test_run_filter_threshold(void) {
    FILE* input = make_ppm("P3\n2 1\n255\n0 0 0 128 128 128\n");
    FILE* output = tmpfile();
    char* argv[] = {"prog", "--threshold", "100"};
    int code = run_filter(3, argv, input, output);

    check(code == EC_OK, "run_filter --threshold -> exit 0");

    rewind(output);
    struct PpmResult result = ppm_read(output);
    check(result.error == PRE_OK, "run_filter --threshold: output readable");
    if (result.error == PRE_OK) {
        check_pixel(&result.image.pixels[0], 0, 0, 0, "run_filter --threshold: pixel 0 black");
        check_pixel(&result.image.pixels[1], 255, 255, 255,
                    "run_filter --threshold: pixel 1 white");
        ppm_image_free(&result.image);
    }

    fclose(input);
    fclose(output);
}

static void test_run_filter_empty(void) {
    FILE* input = make_ppm("");
    FILE* output = tmpfile();
    char* argv[] = {"prog", "--grayscale"};
    int code = run_filter(2, argv, input, output);
    check(code == EC_NOINPUT, "run_filter empty -> EC_NOINPUT");
    fclose(input);
    fclose(output);
}

static void test_run_filter_bad_args(void) {
    FILE* input = make_ppm("P3\n1 1\n255\n0 0 0\n");
    FILE* output = tmpfile();
    char* argv[] = {"prog", "--invalid"};
    int code = run_filter(2, argv, input, output);
    check(code == EC_USAGE, "run_filter bad args -> EC_USAGE");
    fclose(input);
    fclose(output);
}

static void test_run_filter_help(void) {
    FILE* input = make_ppm("");
    FILE* output = tmpfile();
    char* argv[] = {"prog", "--help"};
    int code = run_filter(2, argv, input, output);
    check(code == EC_OK, "run_filter --help -> EC_OK");

    rewind(output);
    char buf[256];
    size_t n = fread(buf, 1, sizeof(buf) - 1, output);
    buf[n] = '\0';
    check(strstr(buf, "Использование") != NULL, "run_filter --help: usage in stdout");
    fclose(input);
    fclose(output);
}

static void test_run_filter_version(void) {
    FILE* input = make_ppm("");
    FILE* output = tmpfile();
    char* argv[] = {"prog", "--version"};
    int code = run_filter(2, argv, input, output);
    check(code == EC_OK, "run_filter --version -> EC_OK");

    rewind(output);
    char buf[256];
    size_t n = fread(buf, 1, sizeof(buf) - 1, output);
    buf[n] = '\0';
    check(strstr(buf, "filter") != NULL, "run_filter --version: name in stdout");
    fclose(input);
    fclose(output);
}

// -------------------------------------------------------------------
// main
// -------------------------------------------------------------------

int main(void) {
    printf("--- luma tests (C) ---\n");
    test_luma_black();
    test_luma_red();
    test_luma_green();

    printf("--- grayscale tests (C) ---\n");
    test_grayscale_black();
    test_grayscale_red();
    test_grayscale_mixed();

    printf("--- threshold tests (C) ---\n");
    test_threshold_above();
    test_threshold_below();
    test_threshold_boundary();

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

    printf("--- run_filter integration tests (C) ---\n");
    test_run_filter_grayscale();
    test_run_filter_threshold();
    test_run_filter_empty();
    test_run_filter_bad_args();
    test_run_filter_help();
    test_run_filter_version();

    printf("---\n");
    if (failed > 0)
        fprintf(stderr, "%d tests FAILED\n", failed);
    else
        printf("All tests PASSED\n");

    return failed > 0 ? 1 : 0;
}
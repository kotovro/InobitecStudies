#include "ppm_stats.h"

#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// luma определена с external linkage в ppm_stats.c
double luma(int32_t r, int32_t g, int32_t b);

static int failed = 0;

static void check(int cond, const char* name) {
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", name);
        ++failed;
    } else {
        printf("PASS: %s\n", name);
    }
}

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

// Header tests

static void test_empty_input(void) {
    FILE* f = make_ppm("");
    struct StatsResult r = ppm_read_stats(f);
    check(r.error == SE_EMPTY_INPUT, "empty input -> SE_EMPTY_INPUT");
    fclose(f);
}

static void test_magic_bad(void) {
    FILE* f = make_ppm("P5\n1 1\n255\n0 0 0\n");
    struct StatsResult r = ppm_read_stats(f);
    check(r.error == SE_BAD_MAGIC, "P5 -> SE_BAD_MAGIC");
    fclose(f);
}

static void test_magic_truncated(void) {
    FILE* f = make_ppm("P");
    struct StatsResult r = ppm_read_stats(f);
    check(r.error == SE_BAD_MAGIC, "just 'P' -> SE_BAD_MAGIC");
    fclose(f);
}

static void test_magic_ok(void) {
    FILE* f = make_ppm("P3\n1 1\n255\n128 128 128\n");
    struct StatsResult r = ppm_read_stats(f);
    check(r.error == SE_OK, "valid P3 -> SE_OK");
    if (r.error == SE_OK) {
        check(r.stats.width == 1, "width == 1");
        check(r.stats.height == 1, "height == 1");
        check(r.stats.max_val == 255, "max_val == 255");
        check(r.stats.pixel_count == 1, "pixel_count == 1");
    }
    fclose(f);
}

static void test_width_bad(void) {
    FILE* f = make_ppm("P3\n0 1\n255\n0 0 0\n");
    struct StatsResult r = ppm_read_stats(f);
    check(r.error == SE_BAD_NUMBER, "width=0 -> SE_BAD_NUMBER");
    fclose(f);
}

static void test_maxval_not_255(void) {
    FILE* f = make_ppm("P3\n1 1\n100\n0 0 0\n");
    struct StatsResult r = ppm_read_stats(f);
    check(r.error == SE_BAD_NUMBER, "maxval=100 -> SE_BAD_NUMBER");
    if (r.error == SE_BAD_NUMBER)
        check(strstr(r.diagnostic, "255") != NULL, "diagnostic mentions 255");
    fclose(f);
}

// Comment tests

static void test_comment_header_only(void) {
    // Comments only in header area (before maxval)
    FILE* f = make_ppm("P3\n# width and height\n2 2\n# before maxval\n255\n"
                       "0 0 0\n0 0 0\n0 0 0\n0 0 0\n");
    struct StatsResult r = ppm_read_stats(f);
    check(r.error == SE_OK, "comments in header -> SE_OK");
    if (r.error == SE_OK) {
        check(r.stats.width == 2, "comments: width == 2");
        check(r.stats.height == 2, "comments: height == 2");
        check(r.stats.pixel_count == 4, "comments: pixel_count == 4");
    }
    fclose(f);
}

//  Pixel data tests

static void test_channel_out_of_range(void) {
    FILE* f = make_ppm("P3\n1 1\n255\n256 0 0\n");
    struct StatsResult r = ppm_read_stats(f);
    check(r.error == SE_CHANNEL_RANGE, "channel 256 -> SE_CHANNEL_RANGE");
    fclose(f);
}

static void test_channel_negative(void) {
    FILE* f = make_ppm("P3\n1 1\n255\n-1 0 0\n");
    struct StatsResult r = ppm_read_stats(f);
    check(r.error == SE_BAD_NUMBER, "channel -1 -> SE_BAD_NUMBER");
    fclose(f);
}

static void test_not_a_number(void) {
    FILE* f = make_ppm("P3\n1 1\n255\nx 0 0\n");
    struct StatsResult r = ppm_read_stats(f);
    check(r.error == SE_BAD_NUMBER, "not a number -> SE_BAD_NUMBER");
    fclose(f);
}

static void test_too_many_pixels(void) {
    FILE* f = make_ppm("P3\n1 1\n255\n0 0 0 255 255 255\n");
    struct StatsResult r = ppm_read_stats(f);
    check(r.error == SE_TOO_MANY_PIXELS, "extra pixel -> SE_TOO_MANY_PIXELS");
    fclose(f);
}

static void test_too_few_pixels(void) {
    FILE* f = make_ppm("P3\n2 2\n255\n0 0 0 255 0 0 0 255 0\n");
    struct StatsResult r = ppm_read_stats(f);
    check(r.error == SE_TOO_FEW_PIXELS, "missing pixel -> SE_TOO_FEW_PIXELS");
    fclose(f);
}

// Statistics accuracy tests

static void test_stats_known(void) {
    // 2x2 image:
    // (0,0,0)   -> luma=0
    // (255,0,0) -> luma=0.299*255 = 76.245
    // (0,255,0) -> luma=0.587*255 = 149.685
    // (0,0,255) -> luma=0.114*255 = 29.07
    // avg: (63.75, 63.75, 63.75) -> (64, 64, 64)
    // y_min=0, y_max=149.7
    // histogram: bin0=2 (0, 29), bin1=0, bin2=1 (76), bin3=0, bin4=1 (150),
    // bin5-7=0
    FILE* f = make_ppm("P3\n2 2\n255\n"
                       "0 0 0 255 0 0\n"
                       "0 255 0 0 0 255\n");
    struct StatsResult r = ppm_read_stats(f);
    check(r.error == SE_OK, "known image -> SE_OK");
    if (r.error == SE_OK) {
        check(r.stats.width == 2, "known: width");
        check(r.stats.height == 2, "known: height");
        check(r.stats.pixel_count == 4, "known: pixel_count");
        check(r.stats.total_r == 255, "known: total_r == 255");
        check(r.stats.total_g == 255, "known: total_g == 255");
        check(r.stats.total_b == 255, "known: total_b == 255");
        check(r.stats.y_min >= 0.0 && r.stats.y_min <= 0.1, "known: y_min ~ 0.0");
        check(r.stats.y_max >= 149.5 && r.stats.y_max <= 149.9, "known: y_max ~ 149.7");
        check(r.stats.histogram[0] == 2, "known: hist[0] == 2");
        check(r.stats.histogram[2] == 1, "known: hist[2] == 1");
        check(r.stats.histogram[4] == 1, "known: hist[4] == 1");
    }
    fclose(f);
}

// luma unit tests

static void test_luma(void) {
    check(luma(255, 0, 0) >= 76.2 && luma(255, 0, 0) <= 76.3, "luma(255,0,0) ~ 76.245");
    check(luma(0, 255, 0) >= 149.6 && luma(0, 255, 0) <= 149.7, "luma(0,255,0) ~ 149.685");
    check(luma(0, 0, 255) >= 29.0 && luma(0, 0, 255) <= 29.1, "luma(0,0,255) ~ 29.07");
    check(luma(255, 255, 255) >= 254.9 && luma(255, 255, 255) <= 255.1,
          "luma(255,255,255) == 255.0");
    check(luma(0, 0, 0) >= -0.1 && luma(0, 0, 0) <= 0.1, "luma(0,0,0) == 0.0");
    check(luma(128, 128, 128) >= 127.9 && luma(128, 128, 128) <= 128.1,
          "luma(128,128,128) ~ 128.0");
}

// main

int main(void) {
    setlocale(LC_ALL, "Russian_Russia.1251");

    printf("--- ppm_read_stats tests ---\n");

    printf("-- header errors --\n");
    test_empty_input();
    test_magic_bad();
    test_magic_truncated();
    test_magic_ok();
    test_width_bad();
    test_maxval_not_255();

    printf("-- comments --\n");
    test_comment_header_only();

    printf("-- pixel data errors --\n");
    test_channel_out_of_range();
    test_channel_negative();
    test_not_a_number();
    test_too_many_pixels();
    test_too_few_pixels();

    printf("-- luma --\n");
    test_luma();

    printf("-- statistics --\n");
    test_stats_known();

    printf("---\n");
    if (failed > 0)
        fprintf(stderr, "%d tests FAILED\n", failed);
    else
        printf("All tests PASSED\n");

    return failed > 0 ? 1 : 0;
}
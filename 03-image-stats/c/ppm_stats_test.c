#include "ppm_stats.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../../common/c/luma.h"

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

// Statistics accuracy tests (input built through ppm_read)

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
    struct PpmResult pr = ppm_read(f);
    fclose(f);
    check(pr.error == PRE_OK, "known image -> PRE_OK");
    if (pr.error != PRE_OK)
        return;

    struct Stats s;
    compute_stats(&pr.image, &s);
    ppm_image_free(&pr.image);

    check(s.width == 2, "known: width");
    check(s.height == 2, "known: height");
    check(s.pixel_count == 4, "known: pixel_count");
    check(s.total_r == 255, "known: total_r == 255");
    check(s.total_g == 255, "known: total_g == 255");
    check(s.total_b == 255, "known: total_b == 255");
    check(s.y_min >= 0.0 && s.y_min <= 0.1, "known: y_min ~ 0.0");
    check(s.y_max >= 149.5 && s.y_max <= 149.9, "known: y_max ~ 149.7");
    check(s.histogram[0] == 2, "known: hist[0] == 2");
    check(s.histogram[2] == 1, "known: hist[2] == 1");
    check(s.histogram[4] == 1, "known: hist[4] == 1");
}

static void test_stats_comments(void) {
    FILE* f = make_ppm("P3\n# width and height\n2 2\n# before maxval\n255\n"
                       "0 0 0\n0 0 0\n0 0 0\n0 0 0\n");
    struct PpmResult pr = ppm_read(f);
    fclose(f);
    check(pr.error == PRE_OK, "comments in header -> PRE_OK");
    if (pr.error != PRE_OK)
        return;

    struct Stats s;
    compute_stats(&pr.image, &s);
    ppm_image_free(&pr.image);

    check(s.width == 2, "comments: width == 2");
    check(s.height == 2, "comments: height == 2");
    check(s.pixel_count == 4, "comments: pixel_count == 4");
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
    printf("--- compute_stats tests ---\n");
    test_stats_known();
    test_stats_comments();

    printf("-- luma --\n");
    test_luma();

    printf("---\n");
    if (failed > 0)
        fprintf(stderr, "%d tests FAILED\n", failed);
    else
        printf("All tests PASSED\n");

    return failed > 0 ? 1 : 0;
}
#include "ppm_io.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "luma.h"

static int failed = 0;

static void check(int cond, const char* name) {
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", name);
        ++failed;
    } else {
        printf("PASS: %s\n", name);
    }
}

static void check_pixel(const struct Pixel* p, int er, int eg, int eb, const char* name) {
    if (p->r != er || p->g != eg || p->b != eb) {
        fprintf(stderr, "FAIL: %s -- got (%d,%d,%d) expected (%d,%d,%d)\n", name, p->r, p->g, p->b,
                er, eg, eb);
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

// -------------------------------------------------------------------
// ppm_read tests
// -------------------------------------------------------------------

static void test_empty_input(void) {
    FILE* f = make_ppm("");
    struct PpmResult r = ppm_read(f);
    check(r.error == PRE_EMPTY_INPUT, "empty input -> PRE_EMPTY_INPUT");
    fclose(f);
}

static void test_magic_bad(void) {
    FILE* f = make_ppm("P5\n1 1\n255\n0 0 0\n");
    struct PpmResult r = ppm_read(f);
    check(r.error == PRE_BAD_MAGIC, "P5 -> PRE_BAD_MAGIC");
    fclose(f);
}

static void test_magic_truncated(void) {
    FILE* f = make_ppm("P");
    struct PpmResult r = ppm_read(f);
    check(r.error == PRE_BAD_MAGIC, "just 'P' -> PRE_BAD_MAGIC");
    fclose(f);
}

static void test_valid_1x1(void) {
    FILE* f = make_ppm("P3\n1 1\n255\n128 128 128\n");
    struct PpmResult r = ppm_read(f);
    check(r.error == PRE_OK, "valid 1x1 -> PRE_OK");
    if (r.error == PRE_OK) {
        check(r.image.width == 1, "1x1: width");
        check(r.image.height == 1, "1x1: height");
        check(r.image.max_val == 255, "1x1: max_val");
        check(r.image.pixels != NULL, "1x1: pixels allocated");
        if (r.image.pixels)
            check_pixel(&r.image.pixels[0], 128, 128, 128, "1x1: pixel value");
        ppm_image_free(&r.image);
    }
    fclose(f);
}

static void test_valid_2x2(void) {
    FILE* f = make_ppm("P3\n2 2\n255\n"
                       "0 0 0 255 0 0\n"
                       "0 255 0 0 0 255\n");
    struct PpmResult r = ppm_read(f);
    check(r.error == PRE_OK, "valid 2x2 -> PRE_OK");
    if (r.error == PRE_OK) {
        check(r.image.width == 2, "2x2: width");
        check(r.image.height == 2, "2x2: height");
        check(r.image.pixels != NULL, "2x2: pixels allocated");
        if (r.image.pixels) {
            check_pixel(&r.image.pixels[0], 0, 0, 0, "2x2: pixel 0");
            check_pixel(&r.image.pixels[1], 255, 0, 0, "2x2: pixel 1");
            check_pixel(&r.image.pixels[2], 0, 255, 0, "2x2: pixel 2");
            check_pixel(&r.image.pixels[3], 0, 0, 255, "2x2: pixel 3");
        }
        ppm_image_free(&r.image);
    }
    fclose(f);
}

static void test_comments_in_header(void) {
    FILE* f = make_ppm("P3\n# width height\n2 2\n# before maxval\n255\n"
                       "0 0 0\n0 0 0\n0 0 0\n0 0 0\n");
    struct PpmResult r = ppm_read(f);
    check(r.error == PRE_OK, "comments in header -> PRE_OK");
    if (r.error == PRE_OK) {
        check(r.image.width == 2, "comments: width");
        check(r.image.height == 2, "comments: height");
        check(r.image.max_val == 255, "comments: max_val");
        ppm_image_free(&r.image);
    }
    fclose(f);
}

static void test_maxval_not_255(void) {
    FILE* f = make_ppm("P3\n1 1\n100\n0 0 0\n");
    struct PpmResult r = ppm_read(f);
    check(r.error == PRE_BAD_NUMBER, "maxval=100 -> PRE_BAD_NUMBER");
    fclose(f);
}

static void test_width_zero(void) {
    FILE* f = make_ppm("P3\n0 1\n255\n0 0 0\n");
    struct PpmResult r = ppm_read(f);
    check(r.error == PRE_BAD_NUMBER, "width=0 -> PRE_BAD_NUMBER");
    fclose(f);
}

static void test_channel_out_of_range(void) {
    FILE* f = make_ppm("P3\n1 1\n255\n256 0 0\n");
    struct PpmResult r = ppm_read(f);
    check(r.error == PRE_CHANNEL_RANGE, "channel 256 -> PRE_CHANNEL_RANGE");
    fclose(f);
}

static void test_channel_negative(void) {
    FILE* f = make_ppm("P3\n1 1\n255\n-1 0 0\n");
    struct PpmResult r = ppm_read(f);
    check(r.error == PRE_BAD_NUMBER, "channel -1 -> PRE_BAD_NUMBER");
    fclose(f);
}

static void test_not_a_number(void) {
    FILE* f = make_ppm("P3\n1 1\n255\nx 0 0\n");
    struct PpmResult r = ppm_read(f);
    check(r.error == PRE_BAD_NUMBER, "not a number -> PRE_BAD_NUMBER");
    fclose(f);
}

static void test_hash_in_data(void) {
    FILE* f = make_ppm("P3\n1 1\n255\n0 0 0 # trailing\n");
    struct PpmResult r = ppm_read(f);
    check(r.error == PRE_BAD_NUMBER, "hash in data -> PRE_BAD_NUMBER");
    fclose(f);
}

static void test_too_many_pixels(void) {
    FILE* f = make_ppm("P3\n1 1\n255\n0 0 0 255 255 255\n");
    struct PpmResult r = ppm_read(f);
    check(r.error == PRE_TOO_MANY_PIXELS, "extra pixel -> PRE_TOO_MANY_PIXELS");
    fclose(f);
}

static void test_too_few_pixels(void) {
    FILE* f = make_ppm("P3\n2 2\n255\n0 0 0 255 0 0 0 255 0\n");
    struct PpmResult r = ppm_read(f);
    check(r.error == PRE_TOO_FEW_PIXELS, "missing pixel -> PRE_TOO_FEW_PIXELS");
    fclose(f);
}

static void test_error_line(void) {
    FILE* f = make_ppm("P3\n1 1\n255\nx 0 0\n");
    struct PpmResult r = ppm_read(f);
    check(r.error == PRE_BAD_NUMBER, "not a number -> PRE_BAD_NUMBER");
    check(r.error_line == 4, "bad number reported on line 4");
    fclose(f);

    f = make_ppm("P3\n1 1\n255\n256 0 0\n");
    r = ppm_read(f);
    check(r.error == PRE_CHANNEL_RANGE, "channel 256 -> PRE_CHANNEL_RANGE");
    check(r.error_line == 4, "channel range reported on line 4");
    fclose(f);

    f = make_ppm("P3\n1 1\n100\n0 0 0\n");
    r = ppm_read(f);
    check(r.error == PRE_BAD_NUMBER, "maxval=100 -> PRE_BAD_NUMBER");
    check(r.error_line == 3, "maxval reported on line 3");
    fclose(f);
}

// -------------------------------------------------------------------
// Writer tests
// -------------------------------------------------------------------

static void test_writer_basic(void) {
    FILE* f = tmpfile();
    check(f != NULL, "writer: tmpfile created");
    if (!f)
        return;

    struct PpmWriter pw;
    ppm_writer_init(&pw, f, 2, 2);
    ppm_writer_put(&pw, (uint8_t)10, (uint8_t)20, (uint8_t)30);
    ppm_writer_put(&pw, (uint8_t)40, (uint8_t)50, (uint8_t)60);
    ppm_writer_put(&pw, (uint8_t)70, (uint8_t)80, (uint8_t)90);
    ppm_writer_put(&pw, (uint8_t)100, (uint8_t)110, (uint8_t)120);

    rewind(f);
    // Read back
    struct PpmResult r = ppm_read(f);
    check(r.error == PRE_OK, "writer: read back ok");
    if (r.error == PRE_OK) {
        check(r.image.width == 2, "writer: width");
        check(r.image.height == 2, "writer: height");
        check(r.image.pixels != NULL, "writer: pixels allocated");
        if (r.image.pixels) {
            check_pixel(&r.image.pixels[0], 10, 20, 30, "writer: pixel 0");
            check_pixel(&r.image.pixels[1], 40, 50, 60, "writer: pixel 1");
            check_pixel(&r.image.pixels[2], 70, 80, 90, "writer: pixel 2");
            check_pixel(&r.image.pixels[3], 100, 110, 120, "writer: pixel 3");
        }
        ppm_image_free(&r.image);
    }
    fclose(f);
}

static void test_image_free_null(void) {
    // ppm_image_free must handle NULL gracefully
    ppm_image_free(NULL);
    check(1, "ppm_image_free(NULL) -> no crash");

    // Also test freeing a zero-initialized Image
    struct Image img = {0, 0, 0, NULL};
    ppm_image_free(&img);
    check(1, "ppm_image_free(zeroed Image) -> no crash");
}

static void test_writer_finish_error(void) {
    // Writing to a read-only FILE* fails (fprintf < 0) and sets ferror;
    // ppm_writer_finish must surface the failure.
    FILE* wf = fopen("_ppm_io_finish_test.tmp", "w");
    if (!wf) {
        check(0, "finish error: cannot create temp file");
        return;
    }
    fputs("x", wf);
    fclose(wf);

    FILE* ro = fopen("_ppm_io_finish_test.tmp", "r");
    if (!ro) {
        check(0, "finish error: cannot reopen temp file for read");
        remove("_ppm_io_finish_test.tmp");
        return;
    }

    struct PpmWriter w;
    ppm_writer_init(&w, ro, 1, 1);
    ppm_writer_put(&w, (uint8_t)1, (uint8_t)2, (uint8_t)3);
    check(ppm_writer_finish(&w) != 0, "finish detects write failure");

    fclose(ro);
    remove("_ppm_io_finish_test.tmp");
}

// -------------------------------------------------------------------
// luma tests
// -------------------------------------------------------------------

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

// -------------------------------------------------------------------
// main
// -------------------------------------------------------------------

int main(void) {
    printf("--- ppm_read tests (C) ---\n");

    printf("-- header errors --\n");
    test_empty_input();
    test_magic_bad();
    test_magic_truncated();
    test_valid_1x1();
    test_maxval_not_255();
    test_width_zero();
    test_comments_in_header();

    printf("-- pixel data errors --\n");
    test_channel_out_of_range();
    test_channel_negative();
    test_not_a_number();
    test_hash_in_data();
    test_too_many_pixels();
    test_too_few_pixels();
    test_valid_2x2();
    test_error_line();

    printf("-- writer --\n");
    test_writer_basic();
    test_image_free_null();
    test_writer_finish_error();

    printf("-- luma --\n");
    test_luma();

    printf("---\n");
    if (failed > 0)
        fprintf(stderr, "%d tests FAILED\n", failed);
    else
        printf("All tests PASSED\n");

    return failed > 0 ? 1 : 0;
}
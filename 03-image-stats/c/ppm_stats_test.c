#include "ppm_stats.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../../common/c/exit_codes.h"
#include "../../common/c/strerror.h"

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

static FILE* open_file(const char* path, const char* mode) {
#ifdef _MSC_VER
    FILE* f = NULL;
    return fopen_s(&f, path, mode) == 0 ? f : NULL;
#else
    return fopen(path, mode);
#endif
}

static FILE* make_readonly_output(void) {
    FILE* wf = open_file("_ppm_stats_write_test.tmp", "w");
    if (!wf)
        return NULL;
    fclose(wf);
    return open_file("_ppm_stats_write_test.tmp", "r");
}

static void check_io_diagnostic(const struct StatsWriteResult* r, int expected_errno,
                                const char* name) {
    char detail[160];
    char expected_errno_text[32];
    char sys[128];

    snprintf(detail, sizeof detail, "%s: error", name);
    check(r->error == STATS_WRITE_IO_ERROR, detail);
    if (r->error != STATS_WRITE_IO_ERROR)
        return;
    snprintf(detail, sizeof detail, "%s: prefix", name);
    check(strstr(r->diagnostic, "сбой записи отчёта") != NULL, detail);
    snprintf(expected_errno_text, sizeof expected_errno_text, "(errno %d)", expected_errno);
    snprintf(detail, sizeof detail, "%s: errno", name);
    check(strstr(r->diagnostic, expected_errno_text) != NULL, detail);
    safe_strerror(expected_errno, sys, sizeof sys);
    snprintf(detail, sizeof detail, "%s: system text", name);
    check(strstr(r->diagnostic, sys) != NULL, detail);
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

static void test_write_error_message(void) {
    char buf[PPM_DIAG_SIZE];
    char sys[128];

    safe_strerror(ENOSPC, sys, sizeof sys);
    stats_write_error_message(ENOSPC, buf, sizeof buf);
    check(strstr(buf, "сбой записи отчёта") != NULL, "write message: prefix");
    check(strstr(buf, sys) != NULL, "write message: system text");
    check(strstr(buf, "(errno 28)") != NULL, "write message: errno");

    stats_write_error_message(0, buf, sizeof buf);
    check(strstr(buf, "неизвестная ошибка ввода-вывода") != NULL, "write message: fallback");
}

static void test_print_stats_write_error(void) {
    struct Stats s = {0};
    s.width = 1;
    s.height = 1;
    s.max_val = 255;
    s.pixel_count = 1;
    s.total_r = 1;
    s.total_g = 2;
    s.total_b = 3;
    s.histogram[0] = 1;

    FILE* out = make_readonly_output();
    if (!out) {
        check(0, "print stats write error: cannot create output");
        return;
    }

    struct StatsWriteResult r = ppm_print_stats(&s, out);
    fclose(out);
    remove("_ppm_stats_write_test.tmp");
    check_io_diagnostic(&r, EBADF, "print stats write error");
}

static void test_run_stats_write_error(void) {
    FILE* in = make_ppm("P3\n1 1\n255\n0 0 0\n");
    FILE* out = make_readonly_output();
    if (!in || !out) {
        check(0, "run stats write error: cannot create streams");
        if (in)
            fclose(in);
        if (out)
            fclose(out);
        remove("_ppm_stats_write_test.tmp");
        return;
    }

    int rc = run_stats(in, out);
    fclose(in);
    fclose(out);
    remove("_ppm_stats_write_test.tmp");
    check(rc == EC_IOERR, "run stats write error: EC_IOERR");
}

static void test_run_stats_success(void) {
    FILE* in = make_ppm("P3\n1 1\n255\n0 0 0\n");
    FILE* out = tmpfile();
    if (!in || !out) {
        check(0, "run stats success: cannot create streams");
        if (in)
            fclose(in);
        if (out)
            fclose(out);
        return;
    }

    int rc = run_stats(in, out);
    fclose(in);
    fclose(out);
    check(rc == EC_OK, "run stats success: EC_OK");
}

// main

int main(void) {
    printf("--- compute_stats tests ---\n");
    test_stats_known();
    test_stats_comments();

    printf("--- write error tests ---\n");
    test_write_error_message();
    test_print_stats_write_error();
    test_run_stats_write_error();
    test_run_stats_success();

    printf("---\n");
    if (failed > 0)
        fprintf(stderr, "%d tests FAILED\n", failed);
    else
        printf("All tests PASSED\n");

    return failed > 0 ? 1 : 0;
}
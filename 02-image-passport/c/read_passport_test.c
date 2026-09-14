#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../common/c/strerror.h"
#include "pixel_word.h"
#include "read_passport.h"

static int failed = 0;

static void check(int cond, const char* name) {
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", name);
        ++failed;
    } else {
        printf("PASS: %s\n", name);
    }
}

static FILE* make_input(const char* data) {
    FILE* f = tmpfile();
    if (!f) {
        fprintf(stderr, "FAIL: tmpfile creation failed\n");
        return NULL;
    }
    fputs(data, f);
    rewind(f);
    return f;
}

static void test_read_valid(void) {
    FILE* f = make_input("морской закат\n1920\n");
    struct PassportResult r = read_passport(f);
    check(r.error == PE_OK, "valid input -> PE_OK");
    if (r.error == PE_OK) {
        check(strcmp(r.name, "морской закат") == 0, "valid: name");
        check(r.count == 1920, "valid: count");
        free(r.name);
    }
    fclose(f);
}

static void test_read_no_input(void) {
    FILE* f = make_input("");
    struct PassportResult r = read_passport(f);
    check(r.error == PE_NO_INPUT, "no input -> PE_NO_INPUT");
    fclose(f);
}

static void test_read_empty_name(void) {
    FILE* f = make_input("\n1920\n");
    struct PassportResult r = read_passport(f);
    check(r.error == PE_EMPTY_NAME, "empty name -> PE_EMPTY_NAME");
    fclose(f);
}

static void test_read_bad_count(void) {
    FILE* f = make_input("тест\nabc\n");
    struct PassportResult r = read_passport(f);
    check(r.error == PE_BAD_COUNT, "bad count -> PE_BAD_COUNT");
    check(strcmp(r.bad_value, "abc") == 0, "bad count -> bad_value");
    fclose(f);
}

static void test_read_negative_count(void) {
    FILE* f = make_input("тест\n-5\n");
    struct PassportResult r = read_passport(f);
    check(r.error == PE_NEGATIVE_COUNT, "negative count -> PE_NEGATIVE_COUNT");
    fclose(f);
}

static void test_read_io_error(void) {
    // A write-only stream cannot be read from, so fgets fails without EOF.
    FILE* wf = fopen("_read_passport_io_test.tmp", "w");
    if (!wf) {
        check(0, "io error: cannot create temp file");
        return;
    }
    struct PassportResult r = read_passport(wf);
    check(r.error == PE_IO_ERROR, "io error -> PE_IO_ERROR");
    fclose(wf);
    remove("_read_passport_io_test.tmp");
}

static void test_error_messages(void) {
    char buf[256];

    struct PassportResult no_input = {.error = PE_NO_INPUT};
    passport_error_message(&no_input, buf, sizeof buf);
    check(strcmp(buf, "Нет ввода") == 0, "msg: no input");

    struct PassportResult empty_name = {.error = PE_EMPTY_NAME};
    passport_error_message(&empty_name, buf, sizeof buf);
    check(strcmp(buf, "Название изображения не может быть пустым") == 0, "msg: empty name");

    struct PassportResult bad_count = {.error = PE_BAD_COUNT};
    snprintf(bad_count.bad_value, sizeof bad_count.bad_value, "%s", "abc");
    passport_error_message(&bad_count, buf, sizeof buf);
    check(strcmp(buf, "количество пикселей должно быть числом; получено: abc") == 0,
          "msg: bad count");

    struct PassportResult negative = {.error = PE_NEGATIVE_COUNT};
    snprintf(negative.bad_value, sizeof negative.bad_value, "%s", "-5");
    passport_error_message(&negative, buf, sizeof buf);
    check(strcmp(buf, "количество пикселей должно быть положительным; получено: -5") == 0,
          "msg: negative count");

    struct PassportResult io = {.error = PE_IO_ERROR, .system_errno = EIO};
    passport_error_message(&io, buf, sizeof buf);
    char errbuf[256];
    safe_strerror(EIO, errbuf, sizeof errbuf);
    char expected[256];
    snprintf(expected, sizeof expected, "Сбой ввода: %s (errno %d)", errbuf, EIO);
    check(strcmp(buf, expected) == 0, "msg: io error");
}

int main(void) {
    printf("--- pixel_word tests ---\n");

    check(strcmp(pixel_word(0), "пикселей") == 0, "0 пикселей");
    check(strcmp(pixel_word(1), "пиксель") == 0, "1 пиксель");
    check(strcmp(pixel_word(2), "пикселя") == 0, "2 пикселя");
    check(strcmp(pixel_word(3), "пикселя") == 0, "3 пикселя");
    check(strcmp(pixel_word(4), "пикселя") == 0, "4 пикселя");
    check(strcmp(pixel_word(5), "пикселей") == 0, "5 пикселей");
    check(strcmp(pixel_word(6), "пикселей") == 0, "6 пикселей");
    check(strcmp(pixel_word(10), "пикселей") == 0, "10 пикселей");
    check(strcmp(pixel_word(11), "пикселей") == 0, "11 пикселей");
    check(strcmp(pixel_word(12), "пикселей") == 0, "12 пикселей");
    check(strcmp(pixel_word(13), "пикселей") == 0, "13 пикселей");
    check(strcmp(pixel_word(14), "пикселей") == 0, "14 пикселей");
    check(strcmp(pixel_word(20), "пикселей") == 0, "20 пикселей");
    check(strcmp(pixel_word(21), "пиксель") == 0, "21 пиксель");
    check(strcmp(pixel_word(22), "пикселя") == 0, "22 пикселя");
    check(strcmp(pixel_word(23), "пикселя") == 0, "23 пикселя");
    check(strcmp(pixel_word(24), "пикселя") == 0, "24 пикселя");
    check(strcmp(pixel_word(25), "пикселей") == 0, "25 пикселей");
    check(strcmp(pixel_word(101), "пиксель") == 0, "101 пиксель");
    check(strcmp(pixel_word(102), "пикселя") == 0, "102 пикселя");
    check(strcmp(pixel_word(111), "пикселей") == 0, "111 пикселей");
    check(strcmp(pixel_word(114), "пикселей") == 0, "114 пикселей");
    check(strcmp(pixel_word(121), "пиксель") == 0, "121 пиксель");
    check(strcmp(pixel_word(122), "пикселя") == 0, "122 пикселя");
    check(strcmp(pixel_word(1000), "пикселей") == 0, "1000 пикселей");
    check(strcmp(pixel_word(1001), "пиксель") == 0, "1001 пиксель");
    check(strcmp(pixel_word(2002), "пикселя") == 0, "2002 пикселя");

    printf("--- read_passport tests ---\n");
    test_read_valid();
    test_read_no_input();
    test_read_empty_name();
    test_read_bad_count();
    test_read_negative_count();
    test_read_io_error();

    printf("--- error messages ---\n");
    test_error_messages();

    printf("---\n");
    if (failed > 0)
        fprintf(stderr, "%d tests FAILED\n", failed);
    else
        printf("All tests PASSED\n");

    return failed > 0 ? 1 : 0;
}

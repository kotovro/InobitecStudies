#include <stdio.h>
#include <string.h>

#include "pixel_word.h"

static int failed = 0;

static void check(int cond, const char* name) {
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", name);
        ++failed;
    } else {
        printf("PASS: %s\n", name);
    }
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

    printf("---\n");
    if (failed > 0)
        fprintf(stderr, "%d tests FAILED\n", failed);
    else
        printf("All tests PASSED\n");

    return failed > 0 ? 1 : 0;
}
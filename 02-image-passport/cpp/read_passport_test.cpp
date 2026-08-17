#include <cstdlib>
#include <print>
#include <string_view>

#include "pixel_word.hpp"

static int failed = 0;

static void check(bool cond, std::string_view name) {
    if (!cond) {
        std::println(stderr, "FAIL: {}", name);
        ++failed;
    } else {
        std::println("PASS: {}", name);
    }
}

int main() {
    std::println("--- pixel_word tests ---");

    check(pixel_word(0) == "пикселей", "0 пикселей");
    check(pixel_word(1) == "пиксель", "1 пиксель");
    check(pixel_word(2) == "пикселя", "2 пикселя");
    check(pixel_word(3) == "пикселя", "3 пикселя");
    check(pixel_word(4) == "пикселя", "4 пикселя");
    check(pixel_word(5) == "пикселей", "5 пикселей");
    check(pixel_word(6) == "пикселей", "6 пикселей");
    check(pixel_word(10) == "пикселей", "10 пикселей");
    check(pixel_word(11) == "пикселей", "11 пикселей");
    check(pixel_word(12) == "пикселей", "12 пикселей");
    check(pixel_word(13) == "пикселей", "13 пикселей");
    check(pixel_word(14) == "пикселей", "14 пикселей");
    check(pixel_word(20) == "пикселей", "20 пикселей");
    check(pixel_word(21) == "пиксель", "21 пиксель");
    check(pixel_word(22) == "пикселя", "22 пикселя");
    check(pixel_word(23) == "пикселя", "23 пикселя");
    check(pixel_word(24) == "пикселя", "24 пикселя");
    check(pixel_word(25) == "пикселей", "25 пикселей");
    check(pixel_word(101) == "пиксель", "101 пиксель");
    check(pixel_word(102) == "пикселя", "102 пикселя");
    check(pixel_word(111) == "пикселей", "111 пикселей");
    check(pixel_word(114) == "пикселей", "114 пикселей");
    check(pixel_word(121) == "пиксель", "121 пиксель");
    check(pixel_word(122) == "пикселя", "122 пикселя");
    check(pixel_word(1000) == "пикселей", "1000 пикселей");
    check(pixel_word(1001) == "пиксель", "1001 пиксель");
    check(pixel_word(2002) == "пикселя", "2002 пикселя");

    std::println("---");
    if (failed > 0)
        std::println(stderr, "{} tests FAILED", failed);
    else
        std::println("All tests PASSED");

    return failed > 0 ? 1 : 0;
}
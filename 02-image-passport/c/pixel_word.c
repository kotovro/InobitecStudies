#include "pixel_word.h"

#include <stdint.h>

const char* pixel_word(int32_t n) {
    int32_t mod100 = n % 100;
    if (mod100 >= 11 && mod100 <= 14)
        return "пикселей";

    switch (n % 10) {
    case 1:
        return "пиксель";
    case 2:
    case 3:
    case 4:
        return "пикселя";
    default:
        return "пикселей";
    }
}
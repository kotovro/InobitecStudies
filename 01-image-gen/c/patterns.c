#include "patterns.h"

#include <math.h>

static uint32_t xorshift32(uint32_t* state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

// @pre 0 <= x < size, 0 <= y < size, size >= 1
struct RGB gradient_pixel(int32_t x, int32_t y, int32_t size) {
    int32_t max_coord = (size == 1) ? 1 : (size - 1);
    int32_t r = x * 255 / max_coord;
    int32_t g = (max_coord - y) * 255 / max_coord;
    return (struct RGB){(uint8_t)r, (uint8_t)g, 0};
}

// @pre 0 <= x < size, 0 <= y < size, size >= 1
struct RGB checker_pixel(int32_t x, int32_t y) {
    int32_t v = ((x + y) % 2 == 0) ? 255 : 0;
    return (struct RGB){(uint8_t)v, (uint8_t)v, (uint8_t)v};
}

// @pre 0 <= x < size, 0 <= y < size, size >= 1
struct RGB radial_pixel(int32_t x, int32_t y, int32_t size) {
    double cx = (size - 1) / 2.0;
    double cy = (size - 1) / 2.0;
    double max_dist = sqrt(cx * cx + cy * cy);
    double dx = x - cx;
    double dy = y - cy;
    double dist = sqrt(dx * dx + dy * dy);
    double hue = (max_dist > 0.0) ? (dist / max_dist) * 360.0 : 0.0;
    return hsv_to_rgb(hue, 1.0, 1.0);
}

/* @pre state != NULL; *state == 0 не допускается,
    функция принудительно заменяет его на 42
    (xorshift32 вырождается на нулевом состоянии) */
struct RGB random_pixel(uint32_t* state) {
    if (*state == 0)
        *state = 42;
    uint8_t r = (uint8_t)(xorshift32(state) % 256);
    uint8_t g = (uint8_t)(xorshift32(state) % 256);
    uint8_t b = (uint8_t)(xorshift32(state) % 256);
    return (struct RGB){r, g, b};
}
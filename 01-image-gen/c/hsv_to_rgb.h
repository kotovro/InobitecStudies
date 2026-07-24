#ifndef KV_HSV_TO_RGB_H
#define KV_HSV_TO_RGB_H

#include <stdint.h>

struct RGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct RGB hsv_to_rgb(double h, double s, double v);

#endif

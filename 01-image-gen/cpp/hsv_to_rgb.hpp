#ifndef KV_HSV_TO_RGB_HPP
#define KV_HSV_TO_RGB_HPP

#include <cstdint>

struct RGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

RGB hsv_to_rgb(double hue, double saturation, double value);

#endif
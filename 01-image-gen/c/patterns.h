#ifndef KV_PATTERNS_H
#define KV_PATTERNS_H

#include "hsv_to_rgb.h"

#include <stdint.h>

struct RGB gradient_pixel(int32_t x, int32_t y, int32_t size);
struct RGB checker_pixel(int32_t x, int32_t y, int32_t size);
struct RGB radial_pixel(int32_t x, int32_t y, int32_t size);
struct RGB random_pixel(uint32_t* state);

#endif
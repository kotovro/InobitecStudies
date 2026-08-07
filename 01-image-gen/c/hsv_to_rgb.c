#include "hsv_to_rgb.h"

#include <math.h>

static int clamp(int val, int lo, int hi) {
    if (val < lo)
        return lo;
    if (val > hi)
        return hi;
    return val;
}

struct RGB hsv_to_rgb(double h, double s, double v) {
    double c = v * s;
    double x = c * (1.0 - fabs(fmod(h / 60.0, 2.0) - 1.0));
    double m = v - c;
    double r1 = 0, g1 = 0, b1 = 0;

    switch ((int)(h / 60.0) % 6) {
    case 0:
        r1 = c;
        g1 = x;
        b1 = 0;
        break;
    case 1:
        r1 = x;
        g1 = c;
        b1 = 0;
        break;
    case 2:
        r1 = 0;
        g1 = c;
        b1 = x;
        break;
    case 3:
        r1 = 0;
        g1 = x;
        b1 = c;
        break;
    case 4:
        r1 = x;
        g1 = 0;
        b1 = c;
        break;
    case 5:
        r1 = c;
        g1 = 0;
        b1 = x;
        break;
    }

    struct RGB result;
    result.r = (uint8_t)clamp((int)((r1 + m) * 255.0), 0, 255);
    result.g = (uint8_t)clamp((int)((g1 + m) * 255.0), 0, 255);
    result.b = (uint8_t)clamp((int)((b1 + m) * 255.0), 0, 255);
    return result;
}
#include "hsv_to_rgb.hpp"

#include <algorithm>
#include <cmath>

RGB hsv_to_rgb(double h, double s, double v) {
    double c = v * s;
    double x = c * (1 - std::abs(std::fmod(h / 60.0, 2) - 1));
    double m = v - c;
    double r1 = 0;
    double g1 = 0;
    double b1 = 0;
    switch (static_cast<int>(h / 60.0) % 6) {
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
    return {static_cast<uint8_t>(std::clamp(static_cast<int>((r1 + m) * 255), 0, 255)),
            static_cast<uint8_t>(std::clamp(static_cast<int>((g1 + m) * 255), 0, 255)),
            static_cast<uint8_t>(std::clamp(static_cast<int>((b1 + m) * 255), 0, 255))};
}

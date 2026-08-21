#include "patterns.hpp"

#include <cstdint>
#include <random>

namespace raster::gen {

RGB random_pixel(std::mt19937& rng) {
    std::uniform_int_distribution dist(0, 255);
    int32_t r = dist(rng);
    int32_t g = dist(rng);
    int32_t b = dist(rng);
    return RGB(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
}

RGB gradient_pixel(int32_t x, int32_t y, int32_t size) {
    int32_t max_coord = (size == 1) ? 1 : (size - 1);
    int32_t r = x * 255 / max_coord;
    int32_t g = (max_coord - y) * 255 / max_coord;
    int32_t b = 0;
    return RGB(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
}

RGB checker_pixel(int32_t x, int32_t y) {
    int32_t v = ((x + y) % 2 == 0) ? 255 : 0;
    return RGB(static_cast<uint8_t>(v), static_cast<uint8_t>(v), static_cast<uint8_t>(v));
}

RGB radial_pixel(int32_t x, int32_t y, int32_t size) {
    double cx = (size - 1) / 2.0;
    double cy = (size - 1) / 2.0;
    double max_dist = std::sqrt(cx * cx + cy * cy);
    double dx = x - cx;
    double dy = y - cy;
    double dist = std::sqrt(dx * dx + dy * dy);

    double hue = max_dist > 0 ? (dist / max_dist) * 360.0 : 0.0;
    RGB color = hsv_to_rgb(hue, 1.0, 1.0);
    return color;
}

} // namespace raster::gen

#ifndef KV_PATTERNS_HPP
#define KV_PATTERNS_HPP

#include "hsv_to_rgb.hpp"

#include <cstdint>
#include <random>

namespace raster::gen {

// @pre 0 <= x < size, 0 <= y < size, size >= 1
RGB gradient_pixel(int32_t x, int32_t y, int32_t size);
// @pre 0 <= x < size, 0 <= y < size, size >= 1
RGB checker_pixel(int32_t x, int32_t y);
// @pre 0 <= x < size, 0 <= y < size, size >= 1
RGB radial_pixel(int32_t x, int32_t y, int32_t size);
// @pre rng в валидном состоянии (после конструирования); нулевой seed допустим
RGB random_pixel(std::mt19937& rng);

} // namespace raster::gen

#endif

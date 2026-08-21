#pragma once

#include <cstdint>

namespace raster::common {
[[nodiscard]] inline double luma(int32_t r, int32_t g, int32_t b) noexcept {
    return 0.299 * r + 0.587 * g + 0.114 * b;
}
} // namespace raster::common

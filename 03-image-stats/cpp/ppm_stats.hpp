#ifndef KV_PPM_STATS_HPP
#define KV_PPM_STATS_HPP

#include <cstdint>
#include <expected>
#include <iosfwd>
#include <string>

#include "../../common/cpp/ppm_io.hpp"

namespace raster::stats {

enum class StatsWriteError {
    kIOError,
};

struct StatsWriteResult {
    std::expected<void, StatsWriteError> value;
    std::string diagnostic;
};

struct Stats {
    int32_t width{};
    int32_t height{};
    int32_t max_val{};
    int64_t pixel_count{};
    int64_t total_r{};
    int64_t total_g{};
    int64_t total_b{};
    double y_min{};
    double y_max{};
    int32_t histogram[8]{};
};

Stats compute_stats(const raster::common::Image& img);
[[nodiscard]] StatsWriteResult ppm_print_stats(const Stats& s, std::ostream& os);
int run_stats(std::istream& input, std::ostream& output);

} // namespace raster::stats

#endif

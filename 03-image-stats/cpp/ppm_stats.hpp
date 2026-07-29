#ifndef KV_PPM_STATS_HPP
#define KV_PPM_STATS_HPP

#include <cstdint>
#include <expected>
#include <iosfwd>
#include <string>
#include <string_view>

#include "../../common/cpp/exit_codes.hpp"

enum class StatsError {
    kOk,
    kEmptyInput,
    kBadMagic,
    kBadNumber,
    kChannelRange,
    kTooFewPixels,
    kTooManyPixels,
    kIOError
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

struct StatsResult {
    std::expected<Stats, StatsError> value;
    std::string diagnostic;
};

StatsResult ppm_read_stats(std::istream& is);
void ppm_print_stats(const Stats& s, std::ostream& os);

#endif

#ifndef KV_PPM_STATS_HPP
#define KV_PPM_STATS_HPP

#include <expected>
#include <iosfwd>
#include <string>
#include <string_view>

#include "../../common/exit_codes.hpp"

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
    int width{};
    int height{};
    int max_val{};
    long long pixel_count{};
    long long total_r{};
    long long total_g{};
    long long total_b{};
    double y_min{};
    double y_max{};
    int histogram[8]{};
};

struct StatsResult {
    std::expected<Stats, StatsError> value;
    std::string diagnostic;
};

StatsResult ppm_read_stats(std::istream& is);
void ppm_print_stats(const Stats& s, std::ostream& os);

#endif

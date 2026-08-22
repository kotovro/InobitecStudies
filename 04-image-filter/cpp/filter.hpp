#ifndef KV_FILTER_HPP
#define KV_FILTER_HPP

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <span>
#include <string_view>

#include "../../common/cpp/exit_codes.hpp"
#include "../../common/cpp/ppm_io.hpp"

namespace raster::common {
struct Pixel;
class Image;
} // namespace raster::common

namespace raster::filter {

enum class FilterMode {
    kGrayscale,
    kThreshold,
};

enum class FilterRequest {
    kRun,
    kHelp,
    kVersion,
};

struct FilterArgs {
    FilterMode mode;
    int threshold{};
};

struct FilterParseResult {
    FilterRequest request = FilterRequest::kRun;
    FilterArgs args{};
};

[[nodiscard]] raster::common::Pixel pixel_to_grayscale(const raster::common::Pixel& p);
[[nodiscard]] raster::common::Pixel pixel_threshold(const raster::common::Pixel& p, int threshold);

void apply_grayscale(raster::common::Image& img);
void apply_threshold(raster::common::Image& img, int threshold);

std::optional<FilterParseResult> parse_filter_args(std::span<const std::string_view> args);

void print_filter_usage(std::ostream& os);
void print_filter_version(std::ostream& os);

int run_filter(std::span<const std::string_view> args, std::istream& input, std::ostream& output);

} // namespace raster::filter

#endif

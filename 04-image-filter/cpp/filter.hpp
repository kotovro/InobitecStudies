#ifndef KV_FILTER_HPP
#define KV_FILTER_HPP

#include <cstdint>
#include <iosfwd>
#include <optional>

#include "../../common/cpp/exit_codes.hpp"
#include "../../common/cpp/ppm_io.hpp"

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

[[nodiscard]] inline double luma(uint8_t r, uint8_t g, uint8_t b) noexcept {
    return 0.299 * r + 0.587 * g + 0.114 * b;
}

[[nodiscard]] Pixel pixel_to_grayscale(const Pixel& p);
[[nodiscard]] Pixel pixel_threshold(const Pixel& p, int threshold);

void apply_grayscale(Image& img);
void apply_threshold(Image& img, int threshold);

std::optional<FilterParseResult> parse_filter_args(int argc, char** argv);

void print_filter_usage(std::ostream& os);
void print_filter_version(std::ostream& os);

int run_filter(int argc, char** argv, std::istream& input, std::ostream& output);

#endif

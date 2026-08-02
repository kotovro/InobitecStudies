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

struct FilterArgs {
    FilterMode mode;
    int threshold{};
};

[[nodiscard]] inline double luma(uint8_t r, uint8_t g, uint8_t b) noexcept {
    return 0.299 * r + 0.587 * g + 0.114 * b;
}

[[nodiscard]] Pixel pixel_to_grayscale(const Pixel& p);
[[nodiscard]] Pixel pixel_threshold(const Pixel& p, int threshold);

void apply_grayscale(Image& img);
void apply_threshold(Image& img, int threshold);

std::optional<FilterArgs> parse_filter_args(int argc, char** argv);

int run_filter(int argc, char** argv, std::istream& input, std::ostream& output);

#endif

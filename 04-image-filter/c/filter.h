#ifndef KV_FILTER_H
#define KV_FILTER_H

#include <stdint.h>
#include <stdio.h>

#include "../../common/c/exit_codes.h"
#include "../../common/c/ppm_io.h"

enum FilterMode {
    FILTER_GRAYSCALE,
    FILTER_THRESHOLD,
};

struct FilterArgs {
    enum FilterMode mode;
    int threshold;
};

static inline double luma(uint8_t r, uint8_t g, uint8_t b) {
    return 0.299 * r + 0.587 * g + 0.114 * b;
}

void pixel_to_grayscale(const struct Pixel* src, struct Pixel* dst);
void pixel_threshold(const struct Pixel* src, int threshold, struct Pixel* dst);
void apply_grayscale(struct Image* img);
void apply_threshold(struct Image* img, int threshold);

// Returns 0 on success, -1 on error (diagnostic written to stderr)
int parse_filter_args(int argc, char** argv, struct FilterArgs* out_args);

// Full pipeline. Returns exit code.
int run_filter(int argc, char** argv, FILE* input, FILE* output);

#endif

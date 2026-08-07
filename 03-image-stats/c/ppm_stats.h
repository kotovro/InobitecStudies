#ifndef KV_PPM_STATS_H
#define KV_PPM_STATS_H

#include <stdint.h>
#include <stdio.h>

#include "../../common/c/ppm_io.h"

struct Stats {
    int32_t width;
    int32_t height;
    int32_t max_val;
    int64_t pixel_count;
    int64_t total_r;
    int64_t total_g;
    int64_t total_b;
    double y_min;
    double y_max;
    int32_t histogram[8];
};

void compute_stats(const struct Image* img, struct Stats* out);
void ppm_print_stats(const struct Stats* s, FILE* out);

#endif

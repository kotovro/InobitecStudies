#ifndef KV_PPM_STATS_H
#define KV_PPM_STATS_H

#include <stdint.h>
#include <stdio.h>

#include "../../common/c/exit_codes.h"

#define STATS_DIAG_SIZE 256

enum StatsError {
    SE_OK,
    SE_EMPTY_INPUT,
    SE_BAD_MAGIC,
    SE_BAD_NUMBER,
    SE_CHANNEL_RANGE,
    SE_TOO_FEW_PIXELS,
    SE_TOO_MANY_PIXELS,
    SE_IO_ERROR
};

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

struct StatsResult {
    enum StatsError error;
    char diagnostic[STATS_DIAG_SIZE];
    struct Stats stats;
};

struct StatsResult ppm_read_stats(FILE* f);
void ppm_print_stats(const struct Stats* s, FILE* out);

#endif

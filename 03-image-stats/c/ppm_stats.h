#ifndef KV_PPM_STATS_H
#define KV_PPM_STATS_H

#include <stdio.h>

#include "../../common/exit_codes.h"

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
    int width;
    int height;
    int max_val;
    long long pixel_count;
    long long total_r;
    long long total_g;
    long long total_b;
    double y_min;
    double y_max;
    int histogram[8];
};

struct StatsResult {
    enum StatsError error;
    char diagnostic[STATS_DIAG_SIZE];
    struct Stats stats;
};

struct StatsResult ppm_read_stats(FILE* f);
void ppm_print_stats(const struct Stats* s, FILE* out);

#endif

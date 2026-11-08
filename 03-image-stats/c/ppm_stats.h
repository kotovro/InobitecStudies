#ifndef KV_PPM_STATS_H
#define KV_PPM_STATS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../../common/c/ppm_io.h"

enum StatsWriteError {
    STATS_WRITE_OK,
    STATS_WRITE_IO_ERROR,
};

struct StatsWriteResult {
    enum StatsWriteError error;
    char diagnostic[PPM_DIAG_SIZE];
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

void compute_stats(const struct Image* img, struct Stats* out);
void stats_write_error_message(int errnum, char* buf, size_t size);
struct StatsWriteResult ppm_print_stats(const struct Stats* s, FILE* out);
int run_stats(FILE* input, FILE* output);

#endif
#include "ppm_stats.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>

#include "../../common/c/exit_codes.h"
#include "../../common/c/luma.h"
#include "../../common/c/strerror.h"

void compute_stats(const struct Image* img, struct Stats* out) {
    struct Stats s = {0, 0, 0, 0, 0, 0, 0, 0.0, 0.0, {0}};
    s.width = img->width;
    s.height = img->height;
    s.max_val = img->max_val;

    int32_t first_pixel = 1;
    int64_t total_pixels = (int64_t)img->width * img->height;

    for (int64_t i = 0; i < total_pixels; ++i) {
        int32_t r = img->pixels[i].r;
        int32_t g = img->pixels[i].g;
        int32_t b = img->pixels[i].b;

        s.total_r += r;
        s.total_g += g;
        s.total_b += b;

        double y = luma(r, g, b);

        if (first_pixel) {
            s.y_min = y;
            s.y_max = y;
            first_pixel = 0;
        } else {
            if (y < s.y_min)
                s.y_min = y;
            if (y > s.y_max)
                s.y_max = y;
        }

        int32_t bin = (int32_t)(y * 8.0 / (s.max_val + 1));
        if (bin >= 8)
            bin = 7;
        s.histogram[bin]++;

        s.pixel_count++;
    }

    *out = s;
}

void stats_write_error_message(int errnum, char* buf, size_t size) {
    if (errnum != 0) {
        char errbuf[128];
        safe_strerror(errnum, errbuf, sizeof errbuf);
        snprintf(buf, size, "сбой записи отчёта: %s (errno %d)", errbuf, errnum);
    } else {
        snprintf(buf, size, "сбой записи отчёта: неизвестная ошибка ввода-вывода");
    }
}

struct StatsWriteResult ppm_print_stats(const struct Stats* s, FILE* out) {
    struct StatsWriteResult result = {STATS_WRITE_OK, {0}};

    int32_t avg_r = (int32_t)((double)s->total_r / s->pixel_count + 0.5);
    int32_t avg_g = (int32_t)((double)s->total_g / s->pixel_count + 0.5);
    int32_t avg_b = (int32_t)((double)s->total_b / s->pixel_count + 0.5);

    errno = 0;
    fprintf(out, "%dx%d\n", s->width, s->height);
    fprintf(out, "пикселей: %lld\n", s->pixel_count);
    fprintf(out, "средний цвет: %3d %3d %3d\n", avg_r, avg_g, avg_b);
    fprintf(out, "мин. яркость: %.1f\n", s->y_min);
    fprintf(out, "макс. яркость: %.1f\n", s->y_max);
    fprintf(out, "гистограмма яркости:\n");

    for (int32_t i = 0; i < 8; ++i) {
        double bin_width = (double)(s->max_val + 1) / 8.0;
        int32_t lo = (int32_t)(i * bin_width);
        int32_t hi = (i == 7) ? s->max_val : (int32_t)((i + 1) * bin_width) - 1;
        fprintf(out, "  [%3d,%3d]: %d\n", lo, hi, s->histogram[i]);
    }

    if (fflush(out) != 0 || ferror(out)) {
        int e = errno;
        result.error = STATS_WRITE_IO_ERROR;
        stats_write_error_message(e, result.diagnostic, sizeof result.diagnostic);
    }

    return result;
}

int run_stats(FILE* input, FILE* output) {
    struct PpmResult result = ppm_read(input);
    if (result.error != PRE_OK) {
        fprintf(stderr, "%s\n", result.diagnostic);
        if (result.error == PRE_EMPTY_INPUT)
            return EC_NOINPUT;
        if (result.error == PRE_IO_ERROR)
            return EC_IOERR;
        return EC_DATA;
    }

    struct Stats stats;
    compute_stats(&result.image, &stats);
    struct StatsWriteResult write_result = ppm_print_stats(&stats, output);
    ppm_image_free(&result.image);

    if (write_result.error != STATS_WRITE_OK) {
        fprintf(stderr, "%s\n", write_result.diagnostic);
        return EC_IOERR;
    }

    return EC_OK;
}
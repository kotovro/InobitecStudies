#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static double luma(int r, int g, int b) { return 0.299 * r + 0.587 * g + 0.114 * b; }

static char* line_buf = NULL;
static size_t line_cap = 0;
static char* line_pos = NULL;

static int is_space(int c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

static int read_line(void) {
    size_t len = 0;
    for (;;) {
        if (len + 2 > line_cap) {
            size_t new_cap = line_cap ? line_cap * 2 : 256;
            char* new_buf = (char*)realloc(line_buf, new_cap);
            if (!new_buf)
                return 0;
            line_buf = new_buf;
            line_cap = new_cap;
        }
        if (!fgets(line_buf + len, (int)(line_cap - len), stdin)) {
            if (len == 0)
                return 0;
            break;
        }
        size_t got = strlen(line_buf + len);
        len += got;
        if (len > 0 && line_buf[len - 1] == '\n') {
            line_buf[--len] = '\0';
            break;
        }
    }
    line_buf[len] = '\0';
    line_pos = line_buf;
    return 1;
}

static const char* next_token(void) {
    for (;;) {
        if (!line_pos || !*line_pos) {
            if (!read_line())
                return NULL;
        }
        while (is_space((unsigned char)*line_pos))
            ++line_pos;
        if (!*line_pos)
            continue;
        char* start = line_pos;
        while (*line_pos && !is_space((unsigned char)*line_pos))
            ++line_pos;
        if (*line_pos)
            *line_pos++ = '\0';
        return start;
    }
}

static int parse_int(const char* s, long long* out) {
    if (!*s)
        return 0;
    errno = 0;
    char* end = NULL;
    long long v = strtoll(s, &end, 10);
    if (errno == ERANGE || end == s || *end != '\0')
        return 0;
    *out = v;
    return 1;
}

int main(void) {
    const char* magic = next_token();
    if (!magic) {
        fprintf(stderr, "no input\n");
        return 66;
    }
    if (magic[0] != 'P' || magic[1] != '3') {
        fprintf(stderr, "bad magic\n");
        return 65;
    }

    long long width = 0, height = 0, max_val = 0;
    for (int i = 0; i < 3; ++i) {
        const char* tok = next_token();
        long long v = 0;
        long long* dst = (i == 0) ? &width : (i == 1) ? &height : &max_val;
        if (!tok || !parse_int(tok, &v) || v > 0x7FFFFFFF) {
            fprintf(stderr, "bad header\n");
            return 65;
        }
        *dst = v;
    }
    if (width <= 0 || height <= 0) {
        fprintf(stderr, "bad dims\n");
        return 65;
    }
    if (max_val != 255) {
        fprintf(stderr, "maxval not 255\n");
        return 65;
    }

    long long total_pixels = width * height;
    long long total_r = 0, total_g = 0, total_b = 0;
    double y_min = 1e9, y_max = -1e9;
    int histogram[8] = {0};
    long long pixel_count = 0;

    for (long long i = 0; i < total_pixels; ++i) {
        long long rgb[3];
        for (int k = 0; k < 3; ++k) {
            const char* tok = next_token();
            if (!tok || !parse_int(tok, &rgb[k])) {
                fprintf(stderr, "truncated\n");
                return 65;
            }
        }
        if (rgb[0] < 0 || rgb[0] > max_val || rgb[1] < 0 || rgb[1] > max_val || rgb[2] < 0 ||
            rgb[2] > max_val) {
            fprintf(stderr, "bad channel\n");
            return 65;
        }

        total_r += rgb[0];
        total_g += rgb[1];
        total_b += rgb[2];

        double y = luma((int)rgb[0], (int)rgb[1], (int)rgb[2]);
        if (i == 0) {
            y_min = y;
            y_max = y;
        } else {
            if (y < y_min)
                y_min = y;
            if (y > y_max)
                y_max = y;
        }
        int bin = (int)(y * 8.0 / (max_val + 1));
        if (bin >= 8)
            bin = 7;
        histogram[bin]++;
        pixel_count++;
    }

    int avg_r = (int)((double)total_r / pixel_count + 0.5);
    int avg_g = (int)((double)total_g / pixel_count + 0.5);
    int avg_b = (int)((double)total_b / pixel_count + 0.5);
    printf("%dx%d\n", (int)width, (int)height);
    printf("пикселей: %lld\n", pixel_count);
    printf("средний цвет: %3d %3d %3d\n", avg_r, avg_g, avg_b);
    printf("мин. яркость: %.1f\n", y_min);
    printf("макс. яркость: %.1f\n", y_max);
    printf("гистограмма яркости:\n");
    for (int i = 0; i < 8; ++i) {
        double bin_width = (double)(max_val + 1) / 8.0;
        int lo = (int)(i * bin_width);
        int hi = (i == 7) ? (int)max_val : (int)((i + 1) * bin_width) - 1;
        printf("  [%3d,%3d]: %d\n", lo, hi, histogram[i]);
    }

    free(line_buf);
    return 0;
}

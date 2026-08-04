#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

static double luma(int r, int g, int b) { return 0.299 * r + 0.587 * g + 0.114 * b; }

int main(void) {
    setlocale(LC_ALL, "Russian_Russia.1251");
    int width, height, max_val;
    char magic[4];
    if (scanf("%3s", magic) != 1) {
        fprintf(stderr, "no input\n");
        return 66;
    }
    if (magic[0] != 'P' || magic[1] != '3') {
        fprintf(stderr, "bad magic\n");
        return 65;
    }
    if (scanf("%d %d %d", &width, &height, &max_val) != 3) {
        fprintf(stderr, "bad header\n");
        return 65;
    }
    if (width <= 0 || height <= 0) {
        fprintf(stderr, "bad dims\n");
        return 65;
    }
    if (max_val != 255) {
        fprintf(stderr, "maxval not 255\n");
        return 65;
    }
    long long total_pixels = (long long)width * height;
    long long total_r = 0, total_g = 0, total_b = 0;
    double y_min = 1e9, y_max = -1e9;
    int histogram[8] = {0};
    long long pixel_count = 0;
    for (long long i = 0; i < total_pixels; ++i) {
        int r, g, b;
        if (scanf("%d %d %d", &r, &g, &b) != 3) {
            fprintf(stderr, "truncated\n");
            return 65;
        }
        if (r < 0 || r > max_val || g < 0 || g > max_val || b < 0 || b > max_val) {
            fprintf(stderr, "bad channel\n");
            return 65;
        }
        total_r += r;
        total_g += g;
        total_b += b;
        double y = luma(r, g, b);
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
    printf("%dx%d\n", width, height);
    printf("пикселей: %lld\n", pixel_count);
    printf("средний цвет: %3d %3d %3d\n", avg_r, avg_g, avg_b);
    printf("мин. €ркость: %.1f\n", y_min);
    printf("макс. €ркость: %.1f\n", y_max);
    printf("гистограмма €ркости:\n");
    for (int i = 0; i < 8; ++i) {
        double bin_width = (double)(max_val + 1) / 8.0;
        int lo = (int)(i * bin_width);
        int hi = (i == 7) ? max_val : (int)((i + 1) * bin_width) - 1;
        printf("  [%3d,%3d]: %d\n", lo, hi, histogram[i]);
    }
    return 0;
}
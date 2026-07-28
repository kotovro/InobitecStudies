#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int clamp(int val, int lo, int hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

static void hsv_to_rgb(double h, double s, double v, int* r, int* g, int* b) {
    double c = v * s;
    double x = c * (1.0 - fabs(fmod(h / 60.0, 2.0) - 1.0));
    double m = v - c;
    double r1 = 0, g1 = 0, b1 = 0;

    switch ((int)(h / 60.0) % 6) {
    case 0: r1 = c; g1 = x; b1 = 0; break;
    case 1: r1 = x; g1 = c; b1 = 0; break;
    case 2: r1 = 0; g1 = c; b1 = x; break;
    case 3: r1 = 0; g1 = x; b1 = c; break;
    case 4: r1 = x; g1 = 0; b1 = c; break;
    case 5: r1 = c; g1 = 0; b1 = x; break;
    }

    *r = clamp((int)((r1 + m) * 255.0), 0, 255);
    *g = clamp((int)((g1 + m) * 255.0), 0, 255);
    *b = clamp((int)((b1 + m) * 255.0), 0, 255);
}

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    int size = atoi(argv[1]);
    if (size < 1) return 1;

    printf("P3\n%d %d\n255\n", size, size);
    double cx = (size - 1) / 2.0;
    double cy = (size - 1) / 2.0;
    double max_dist = sqrt(cx * cx + cy * cy);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            double dx = x - cx;
            double dy = y - cy;
            double dist = sqrt(dx * dx + dy * dy);
            double hue = (max_dist > 0.0) ? (dist / max_dist) * 360.0 : 0.0;
            int r, g, b;
            hsv_to_rgb(hue, 1.0, 1.0, &r, &g, &b);
            printf("%3d %3d %3d", r, g, b);
            if (x + 1 < size) printf(" ");
        }
        printf("\n");
    }
    return 0;
}

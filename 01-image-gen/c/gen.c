#include "hsv_to_rgb.h"
#include "parse_args.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void write_ppm_header(int32_t width, int32_t height) {
    printf("P3\n%d %d\n255\n", width, height);
}

static uint32_t xorshift32(uint32_t* state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static void draw_random(int32_t size, uint32_t seed) {
    uint32_t state = seed ? seed : 42;
    int32_t max_val = 255;
    for (int32_t y = 0; y < size; ++y) {
        for (int32_t x = 0; x < size; ++x) {
            int32_t r = (int32_t)(xorshift32(&state) % (max_val + 1));
            int32_t g = (int32_t)(xorshift32(&state) % (max_val + 1));
            int32_t b = (int32_t)(xorshift32(&state) % (max_val + 1));
            printf("%3d %3d %3d", r, g, b);
            if (x + 1 < size)
                printf(" ");
        }
        printf("\n");
    }
}

static void draw_gradient(int32_t size) {
    int32_t max_coord = (size == 1) ? 1 : (size - 1);
    for (int32_t y = 0; y < size; ++y) {
        for (int32_t x = 0; x < size; ++x) {
            int32_t r = x * 255 / max_coord;
            int32_t g = (max_coord - y) * 255 / max_coord;
            int32_t b = 0;
            printf("%3d %3d %3d", r, g, b);
            if (x + 1 < size)
                printf(" ");
        }
        printf("\n");
    }
}

static void draw_checker(int32_t size) {
    for (int32_t y = 0; y < size; ++y) {
        for (int32_t x = 0; x < size; ++x) {
            int32_t v = ((x + y) % 2 == 0) ? 255 : 0;
            printf("%3d %3d %3d", v, v, v);
            if (x + 1 < size)
                printf(" ");
        }
        printf("\n");
    }
}

static void draw_radial(int32_t size) {
    double cx = (size - 1) / 2.0;
    double cy = (size - 1) / 2.0;
    double max_dist = sqrt(cx * cx + cy * cy);

    for (int32_t y = 0; y < size; ++y) {
        for (int32_t x = 0; x < size; ++x) {
            double dx = x - cx;
            double dy = y - cy;
            double dist = sqrt(dx * dx + dy * dy);

            double hue = (max_dist > 0.0) ? (dist / max_dist) * 360.0 : 0.0;
            struct RGB color = hsv_to_rgb(hue, 1.0, 1.0);
            printf("%3d %3d %3d", color.r, color.g, color.b);
            if (x + 1 < size)
                printf(" ");
        }
        printf("\n");
    }
}

static void generate_ppm(const struct ParseResult* args) {
    write_ppm_header(args->size, args->size);
    switch (args->pattern) {
    case PATTERN_GRADIENT:
        draw_gradient(args->size);
        break;
    case PATTERN_CHECKER:
        draw_checker(args->size);
        break;
    case PATTERN_RADIAL:
        draw_radial(args->size);
        break;
    case PATTERN_RANDOM:
        draw_random(args->size, args->seed);
        break;
    }
}

int main(int argc, char** argv) {
    struct ParseResult args = parse_args(argc, argv);
    if (args.error != PE_OK) {
        switch (args.error) {
        case PE_NOARG:
            fprintf(stderr, "N не указано. Использование: gen_image <N> [pattern]\n");
            return EC_NOINPUT;
        case PE_BADNUMBER:
            fprintf(stderr, "N должно быть целым числом; получено: %s\n", argc >= 2 ? argv[1] : "");
            return EC_USAGE;
        case PE_BADPATTERN:
            fprintf(stderr, "Неизвестный паттерн: %s. Допустимые: gradient, checker, radial\n",
                    argc >= 3 ? argv[2] : "");
            return EC_USAGE;
        }
    }

    if (args.size < 1) {
        fprintf(stderr, "размер должен быть положительным; получено: %d\n", args.size);
        return EC_USAGE;
    }

    if (args.pattern != PATTERN_RANDOM && args.size > 512) {
        fprintf(stderr, "N должно быть в [1; 512]; получено: %d\n", args.size);
        return EC_USAGE;
    }

    generate_ppm(&args);
    return EC_OK;
}
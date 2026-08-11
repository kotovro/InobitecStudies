#include "filter.h"

#include "../../common/c/version.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void pixel_to_grayscale(const struct Pixel* src, struct Pixel* dst) {
    double y = luma(src->r, src->g, src->b);
    uint8_t yb = (uint8_t)(y + 0.5);
    dst->r = yb;
    dst->g = yb;
    dst->b = yb;
}

void pixel_threshold(const struct Pixel* src, int threshold, struct Pixel* dst) {
    double y = luma(src->r, src->g, src->b);
    if (y > threshold) {
        dst->r = dst->g = dst->b = 255;
    } else {
        dst->r = dst->g = dst->b = 0;
    }
}

void apply_grayscale(struct Image* img) {
    for (long long i = 0; i < (long long)img->width * img->height; ++i)
        pixel_to_grayscale(&img->pixels[i], &img->pixels[i]);
}

void apply_threshold(struct Image* img, int threshold) {
    for (long long i = 0; i < (long long)img->width * img->height; ++i)
        pixel_threshold(&img->pixels[i], threshold, &img->pixels[i]);
}

int parse_filter_args(int argc, char** argv, struct FilterParseResult* out_result) {
    if (argc < 2) {
        fprintf(stderr, "использование: image_filter --grayscale | --threshold T\n");
        return -1;
    }

    if (strcmp(argv[1], "--help") == 0) {
        out_result->request = FILTER_HELP;
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0) {
        out_result->request = FILTER_VERSION;
        return 0;
    }

    out_result->request = FILTER_RUN;

    if (strcmp(argv[1], "--grayscale") == 0) {
        if (argc > 2) {
            fprintf(stderr, "--grayscale не принимает аргументов\n");
            return -1;
        }
        out_result->args.mode = FILTER_GRAYSCALE;
        return 0;
    }

    if (strcmp(argv[1], "--threshold") == 0) {
        if (argc < 3) {
            fprintf(stderr, "--threshold требует аргумента T\n");
            return -1;
        }

        char* end = NULL;
        long t = strtol(argv[2], &end, 10);
        if (end == argv[2] || *end != '\0') {
            fprintf(stderr, "T должен быть целым числом; получено: %s\n", argv[2]);
            return -1;
        }

        if (t < 0 || t > 255) {
            fprintf(stderr, "T должен быть в [0; 255]; получено: %ld\n", t);
            return -1;
        }

        out_result->args.mode = FILTER_THRESHOLD;
        out_result->args.threshold = (int)t;
        return 0;
    }

    fprintf(stderr, "неизвестный аргумент: %s. используйте --grayscale или --threshold T\n",
            argv[1]);
    return -1;
}

void print_filter_usage(FILE* out) {
    fprintf(out, "Использование: image_filter --grayscale | --threshold T\n");
    fprintf(out, "\n");
    fprintf(out, "Читает PPM P3 из stdin, пишет валидный PPM в stdout.\n");
    fprintf(out, "\n");
    fprintf(out, "Режимы:\n");
    fprintf(out, "  --grayscale     конверсия в оттенки серого по luma\n");
    fprintf(out, "  --threshold T   бинаризация по порогу яркости (0 <= T <= 255)\n");
    fprintf(out, "\n");
    fprintf(out, "Опции:\n");
    fprintf(out, "  --help          показать справку\n");
    fprintf(out, "  --version       показать версию\n");
}

void print_filter_version(FILE* out) { fprintf(out, "image_filter %s\n", KV_VERSION); }

int run_filter(int argc, char** argv, FILE* input, FILE* output) {
    struct FilterParseResult parsed;
    if (parse_filter_args(argc, argv, &parsed) != 0)
        return EC_USAGE;

    if (parsed.request == FILTER_HELP) {
        print_filter_usage(output);
        return EC_OK;
    }

    if (parsed.request == FILTER_VERSION) {
        print_filter_version(output);
        return EC_OK;
    }

    struct FilterArgs args = parsed.args;
    struct PpmResult result = ppm_read(input);
    if (result.error != PRE_OK) {
        fprintf(stderr, "%s\n", result.diagnostic);
        int ec = EC_DATA;
        if (result.error == PRE_EMPTY_INPUT)
            ec = EC_NOINPUT;
        else if (result.error == PRE_IO_ERROR)
            ec = EC_IOERR;
        return ec;
    }

    struct Image* image = &result.image;

    switch (args.mode) {
    case FILTER_GRAYSCALE:
        apply_grayscale(image);
        break;
    case FILTER_THRESHOLD:
        apply_threshold(image, args.threshold);
        break;
    }

    struct PpmWriter writer;
    ppm_writer_init(&writer, output, image->width, image->height);
    for (long long i = 0; i < (long long)image->width * image->height; ++i)
        ppm_writer_put(&writer, image->pixels[i].r, image->pixels[i].g, image->pixels[i].b);

    ppm_image_free(image);
    return EC_OK;
}
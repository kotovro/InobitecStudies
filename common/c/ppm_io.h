#ifndef KV_PPM_IO_H
#define KV_PPM_IO_H

#include <stdint.h>
#include <stdio.h>

#define PPM_DIAG_SIZE 256

struct Pixel {
    uint8_t r, g, b;
};

struct Image {
    int32_t width;
    int32_t height;
    uint16_t max_val;
    struct Pixel* pixels;
};

enum PpmReadError {
    PRE_OK,
    PRE_EMPTY_INPUT,
    PRE_BAD_MAGIC,
    PRE_BAD_NUMBER,
    PRE_CHANNEL_RANGE,
    PRE_TOO_FEW_PIXELS,
    PRE_TOO_MANY_PIXELS,
    PRE_IO_ERROR,
    PRE_ALLOC_ERROR,
};

struct PpmResult {
    enum PpmReadError error;
    char diagnostic[PPM_DIAG_SIZE];
    struct Image image;
};

struct PpmResult ppm_read(FILE* f);
void ppm_image_free(struct Image* img);

struct PpmWriter {
    FILE* f;
    int32_t width;
    int32_t col;
};

void ppm_writer_init(struct PpmWriter* w, FILE* f, int32_t width, int32_t height);
void ppm_writer_put(struct PpmWriter* w, uint8_t r, uint8_t g, uint8_t b);

#endif

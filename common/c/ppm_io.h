#ifndef KV_PPM_IO_H
#define KV_PPM_IO_H

#include <stdint.h>
#include <stdio.h>

enum { PPM_DIAG_SIZE = 256 };

#if defined(_WIN32)
#if defined(KV_DYNAMIC_LINK)
#define KV_API __declspec(dllexport)
#else
#define KV_API
#endif
#else
#define KV_API
#endif

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
    int32_t error_line;
    char diagnostic[PPM_DIAG_SIZE];
    struct Image image;
};

struct PpmResult KV_API ppm_read(FILE* f);
void KV_API ppm_image_free(struct Image* img);

struct PpmWriter {
    FILE* f;
    int32_t width;
    int32_t col;
};

void KV_API ppm_writer_init(struct PpmWriter* w, FILE* f, int32_t width, int32_t height);
void KV_API ppm_writer_put(struct PpmWriter* w, uint8_t r, uint8_t g, uint8_t b);
int KV_API ppm_writer_finish(struct PpmWriter* w);

#endif
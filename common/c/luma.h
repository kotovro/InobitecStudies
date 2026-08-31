#ifndef KV_LUMA_H
#define KV_LUMA_H

#include <stdint.h>

static inline double luma(int32_t r, int32_t g, int32_t b) {
    return 0.299 * r + 0.587 * g + 0.114 * b;
}

#endif

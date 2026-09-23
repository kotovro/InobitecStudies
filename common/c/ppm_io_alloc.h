#ifndef KV_PPM_IO_ALLOC_H
#define KV_PPM_IO_ALLOC_H

#include <stddef.h>
#include <stdio.h>

/*
 * Internal allocation seam for ppm_read. Not part of the public API and not
 * exported (no KV_API). Used by tests to inject a failing/recording allocator.
 *
 * Contract: a custom allocator must return memory compatible with free(),
 * because ppm_image_free releases pixels with free().
 */
struct PpmAllocator {
    void* (*alloc)(size_t size);
    void* (*realloc)(void* ptr, size_t size);
    void (*free)(void* ptr);
};

struct PpmResult ppm_read_with(FILE* f, const struct PpmAllocator* allocator);

#endif

#ifndef KV_STRERROR_H
#define KV_STRERROR_H

#include <stddef.h>

void safe_strerror(int errnum, char* buf, size_t bufsz);

#endif

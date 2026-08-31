#ifndef KV_STRERROR_H
#define KV_STRERROR_H

#include "api.h"

#include <stddef.h>

KV_API void safe_strerror(int errnum, char* buf, size_t bufsz);

#endif

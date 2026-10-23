#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "strerror.h"

#include <stdio.h>
#include <string.h>

void safe_strerror(int errnum, char* buf, size_t bufsz) {
    if (!buf || bufsz == 0)
        return;

#ifdef _WIN32
    strerror_s(buf, bufsz, errnum);
#else
    if (strerror_r(errnum, buf, bufsz) != 0)
        snprintf(buf, bufsz, "Unknown error %d", errnum);
    buf[bufsz - 1] = '\0';
#endif
}

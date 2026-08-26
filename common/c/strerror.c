#include "strerror.h"

#include <string.h>

void safe_strerror(int errnum, char* buf, size_t bufsz) {
#ifdef _WIN32
    strerror_s(buf, bufsz, errnum);
#else
    strerror_r(errnum, buf, bufsz); // POSIX версия
#endif
}

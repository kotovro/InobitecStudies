#include "parse_args.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct ParseResult parse_args(int argc, char** argv) {
    struct ParseResult result = {0, PATTERN_GRADIENT, PE_OK};

    if (argc < 2) {
        result.error = PE_NOARG;
        return result;
    }

    char* end;
    errno = 0;
    long n = strtol(argv[1], &end, 10);
    if (errno == ERANGE || end == argv[1] || *end != '\0') {
        result.error = PE_BADNUMBER;
        return result;
    }
    result.size = (int32_t)n;

    const char* name = (argc >= 3) ? argv[2] : "gradient";
    if (strcmp(name, "gradient") == 0)
        result.pattern = PATTERN_GRADIENT;
    else if (strcmp(name, "checker") == 0)
        result.pattern = PATTERN_CHECKER;
    else if (strcmp(name, "radial") == 0)
        result.pattern = PATTERN_RADIAL;
    else {
        result.error = PE_BADPATTERN;
        return result;
    }

    return result;
}

#include "parse_args.h"

#include <stdlib.h>
#include <string.h>

struct ParseResult parse_args(int argc, char** argv) {
    struct ParseResult result = { 0, PATTERN_GRADIENT, EXIT_OK };

    if (argc < 2) {
        result.error = EXIT_NOINPUT;
        return result;
    }

    char* end;
    long n = strtol(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0') {
        result.error = EXIT_USAGE;
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
        result.error = EXIT_USAGE;
        return result;
    }

    return result;
}

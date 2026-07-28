#ifndef KV_PARSE_ARGS_H
#define KV_PARSE_ARGS_H

#include <stdint.h>

#include "../../common/exit_codes.h"

enum ParseError { PE_OK, PE_NOARG, PE_BADNUMBER, PE_BADPATTERN, PE_BADSEED };

enum Pattern { PATTERN_GRADIENT, PATTERN_CHECKER, PATTERN_RADIAL, PATTERN_RANDOM };

struct ParseResult {
    int32_t size;
    enum Pattern pattern;
    enum ParseError error;
    uint32_t seed;
    int seed_provided;
};

struct ParseResult parse_args(int argc, char** argv);

#endif

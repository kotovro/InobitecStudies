#ifndef KV_PARSE_ARGS_H
#define KV_PARSE_ARGS_H

#include <stdint.h>

#include "../../common/c/exit_codes.h"

enum ParseError { PE_OK, PE_NOARG, PE_BADNUMBER, PE_BADPATTERN, PE_BADSEED };

enum ParseRequest { PR_RUN, PR_HELP, PR_VERSION };

enum Pattern { PATTERN_GRADIENT, PATTERN_CHECKER, PATTERN_RADIAL, PATTERN_RANDOM };

struct ParseResult {
    int32_t size;
    enum Pattern pattern;
    enum ParseError error;
    uint32_t seed;
    int seed_provided;
    enum ParseRequest request;
};

struct ParseResult parse_args(int argc, char** argv);

void print_usage(void);
void print_version(void);

#endif

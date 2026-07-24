#ifndef KV_PARSE_ARGS_H
#define KV_PARSE_ARGS_H

#include <stdint.h>

enum ParseError { PE_OK, PE_NOARG, PE_BADNUMBER, PE_BADPATTERN };

#define EXIT_OK      0
#define EXIT_USAGE   64
#define EXIT_NOINPUT 66

enum Pattern { PATTERN_GRADIENT, PATTERN_CHECKER, PATTERN_RADIAL };

struct ParseResult {
    int32_t size;
    enum Pattern pattern;
    enum ParseError error;
};

struct ParseResult parse_args(int argc, char** argv);

#endif

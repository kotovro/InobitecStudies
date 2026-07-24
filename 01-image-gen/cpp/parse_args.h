#ifndef KV_PARSE_ARGS_H  
#define KV_PARSE_ARGS_H

#include <expected>

enum class Pattern { Gradient, Checker, Radial };
enum class ParseError { kNoArg, kBadNumber, kBadPattern }; 
struct Args { int size; Pattern pattern; }; 
std::expected<Args, ParseError> parse_args(int, char**);

#endif // KV_PARSE_ARGS_H
#ifndef KV_PARSE_ARGS_HPP  
#define KV_PARSE_ARGS_HPP

#include <cstdint>
#include <expected>

#include "../../common/exit_codes.hpp"

enum class Pattern { Gradient, Checker, Radial, Random };
enum class ParseError { kNoArg, kBadNumber, kBadPattern, kBadSeed }; 
struct Args { int size; Pattern pattern; std::uint32_t seed{}; bool seed_provided{}; }; 
std::expected<Args, ParseError> parse_args(int, char**);

#endif // KV_PARSE_ARGS_HPP

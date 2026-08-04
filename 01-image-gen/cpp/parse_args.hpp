#ifndef KV_PARSE_ARGS_HPP
#define KV_PARSE_ARGS_HPP

#include <cstdint>
#include <expected>

#include "../../common/cpp/exit_codes.hpp"

enum class Pattern { Gradient, Checker, Radial, Random };
enum class ParseError { kNoArg, kBadNumber, kBadPattern, kBadSeed };
enum class ParseRequest { kRun, kHelp, kVersion };

struct Args {
    std::int32_t size;
    Pattern pattern;
    std::uint32_t seed{};
    bool seed_provided{};
};

struct ParseResult {
    ParseRequest request = ParseRequest::kRun;
    Args args{};
};

std::expected<ParseResult, ParseError> parse_args(int, char**);

#endif // KV_PARSE_ARGS_HPP

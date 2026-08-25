#ifndef KV_PARSE_ARGS_HPP
#define KV_PARSE_ARGS_HPP

#include <cstdint>
#include <expected>
#include <span>
#include <string_view>

#include "../../common/cpp/exit_codes.hpp"

namespace raster::gen {

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

std::expected<ParseResult, ParseError> parse_args(std::span<const std::string_view> args);

} // namespace raster::gen

#endif // KV_PARSE_ARGS_HPP

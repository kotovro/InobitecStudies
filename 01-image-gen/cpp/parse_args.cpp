#include "parse_args.hpp"

#include <charconv>
#include <string_view>

namespace raster::gen {

namespace {

std::expected<Args, ParseError> parse_args_old(std::span<const std::string_view> args) {
    if (args.size() < 2) [[unlikely]]
        return std::unexpected(ParseError::kNoArg);

    int32_t size{};
    auto [ptr, ec] = std::from_chars(args[1].data(), args[1].data() + args[1].size(), size);
    if (ec != std::errc{} || ptr != args[1].data() + args[1].size()) [[unlikely]]
        return std::unexpected(ParseError::kBadNumber);

    std::string_view name = (args.size() >= 3) ? args[2] : "gradient";
    Pattern pattern{};
    if (name == "gradient")
        pattern = Pattern::Gradient;
    else if (name == "checker")
        pattern = Pattern::Checker;
    else if (name == "radial")
        pattern = Pattern::Radial;
    else [[unlikely]]
        return std::unexpected(ParseError::kBadPattern);

    return Args{.size = size, .pattern = pattern};
}

std::expected<Args, ParseError> parse_args_new(std::span<const std::string_view> args) {
    if (args.size() < 3) [[unlikely]]
        return std::unexpected(ParseError::kNoArg);

    int32_t size{};
    auto [ptr, ec] = std::from_chars(args[2].data(), args[2].data() + args[2].size(), size);
    if (ec != std::errc{} || ptr != args[2].data() + args[2].size()) [[unlikely]]
        return std::unexpected(ParseError::kBadNumber);

    Args result{.size = size, .pattern = Pattern::Random, .seed = 0, .seed_provided = false};

    if (args.size() >= 5 && args[3] == "--seed") {
        std::uint32_t seed{};
        auto [sptr, sec] = std::from_chars(args[4].data(), args[4].data() + args[4].size(), seed);
        if (sec != std::errc{} || sptr != args[4].data() + args[4].size()) [[unlikely]]
            return std::unexpected(ParseError::kBadSeed);
        result.seed = seed;
        result.seed_provided = true;
    }

    return result;
}

} // namespace

std::expected<ParseResult, ParseError> parse_args(std::span<const std::string_view> args) {
    if (args.size() >= 2 && args[1] == "--help")
        return ParseResult{.request = ParseRequest::kHelp};

    if (args.size() >= 2 && args[1] == "--version")
        return ParseResult{.request = ParseRequest::kVersion};

    if (args.size() >= 2 && args[1] == "--size") {
        auto parsed = parse_args_new(args);
        if (!parsed)
            return std::unexpected(parsed.error());
        return ParseResult{.args = *parsed};
    }

    auto parsed = parse_args_old(args);
    if (!parsed)
        return std::unexpected(parsed.error());
    return ParseResult{.args = *parsed};
}

} // namespace raster::gen

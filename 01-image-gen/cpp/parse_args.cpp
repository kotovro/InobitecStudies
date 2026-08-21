#include "parse_args.hpp"

#include <charconv>
#include <cstring>
#include <string_view>

namespace raster::gen {

namespace {

std::expected<Args, ParseError> parse_args_old(int argc, char** argv) {
    if (argc < 2) [[unlikely]]
        return std::unexpected(ParseError::kNoArg);

    int32_t size{};
    auto [ptr, ec] = std::from_chars(argv[1], argv[1] + std::strlen(argv[1]), size);
    if (ec != std::errc{} || *ptr != '\0') [[unlikely]]
        return std::unexpected(ParseError::kBadNumber);

    std::string_view name = (argc >= 3) ? argv[2] : "gradient";
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

std::expected<Args, ParseError> parse_args_new(int argc, char** argv) {
    if (argc < 3) [[unlikely]]
        return std::unexpected(ParseError::kNoArg);

    int32_t size{};
    auto [ptr, ec] = std::from_chars(argv[2], argv[2] + std::strlen(argv[2]), size);
    if (ec != std::errc{} || *ptr != '\0') [[unlikely]]
        return std::unexpected(ParseError::kBadNumber);

    Args result{.size = size, .pattern = Pattern::Random, .seed = 0, .seed_provided = false};

    if (argc >= 5 && std::string_view(argv[3]) == "--seed") {
        std::uint32_t seed{};
        auto [sptr, sec] = std::from_chars(argv[4], argv[4] + std::strlen(argv[4]), seed);
        if (sec != std::errc{} || *sptr != '\0') [[unlikely]]
            return std::unexpected(ParseError::kBadSeed);
        result.seed = seed;
        result.seed_provided = true;
    }

    return result;
}

} // namespace

std::expected<ParseResult, ParseError> parse_args(int argc, char** argv) {
    if (argc >= 2 && std::string_view(argv[1]) == "--help")
        return ParseResult{.request = ParseRequest::kHelp};

    if (argc >= 2 && std::string_view(argv[1]) == "--version")
        return ParseResult{.request = ParseRequest::kVersion};

    if (argc >= 2 && std::string_view(argv[1]) == "--size") {
        auto args = parse_args_new(argc, argv);
        if (!args)
            return std::unexpected(args.error());
        return ParseResult{.args = *args};
    }

    auto args = parse_args_old(argc, argv);
    if (!args)
        return std::unexpected(args.error());
    return ParseResult{.args = *args};
}

} // namespace raster::gen
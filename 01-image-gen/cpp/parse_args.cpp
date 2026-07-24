#include "parse_args.h"

#include <charconv>
#include <cstring>
#include <string_view>

std::expected<Args, ParseError> parse_args(int argc, char** argv) {
    if (argc < 2) [[unlikely]]
        return std::unexpected(ParseError::kNoArg);

    int size{};
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

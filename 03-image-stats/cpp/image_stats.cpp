#include "ppm_stats.hpp"

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <locale>
#include <print>

int main() {
    std::setlocale(LC_ALL, "Russian_Russia.1251");

    auto result = ppm_read_stats(std::cin);
    if (!result.value) {
        switch (result.value.error()) {
        case StatsError::kEmptyInput:
            std::println(stderr, "{}", result.diagnostic);
            return (int)ExitCode::kNoInput;
        case StatsError::kIOError:
            std::println(stderr, "{}", result.diagnostic);
            return (int)ExitCode::kIOErr;
        default:
            std::println(stderr, "{}", result.diagnostic);
            return (int)ExitCode::kData;
        }
    }

    ppm_print_stats(*result.value, std::cout);
    return (int)ExitCode::kOk;
}

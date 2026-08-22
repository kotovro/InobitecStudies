#include "filter.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

using namespace raster::filter;

int main(int argc, char** argv) {
    std::vector<std::string_view> args;
    args.reserve(argc);
    for (int i = 0; i < argc; ++i)
        args.emplace_back(argv[i]);
    return run_filter(args, std::cin, std::cout);
}

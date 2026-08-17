#include "filter.hpp"

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
    return run_filter(argc, argv, std::cin, std::cout);
}
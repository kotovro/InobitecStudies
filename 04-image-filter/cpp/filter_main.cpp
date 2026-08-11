#include "filter.hpp"

#include <cstdlib>
#include <iostream>
#include <locale>

int main(int argc, char** argv) {
    // std::setlocale(LC_ALL, "Russian_Russia.1251");
    return run_filter(argc, argv, std::cin, std::cout);
}
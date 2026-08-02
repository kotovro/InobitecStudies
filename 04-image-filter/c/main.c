#include "filter.h"

#include <locale.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    setlocale(LC_ALL, "Russian_Russia.1251");
    return run_filter(argc, argv, stdin, stdout);
}

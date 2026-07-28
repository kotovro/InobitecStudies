#include "ppm_stats.h"

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    setlocale(LC_ALL, "Russian_Russia.1251");

    struct StatsResult result = ppm_read_stats(stdin);

    switch (result.error) {
    case SE_OK:
        ppm_print_stats(&result.stats, stdout);
        return EC_OK;

    case SE_EMPTY_INPUT:
        fprintf(stderr, "%s\n", result.diagnostic);
        return EC_NOINPUT;

    case SE_IO_ERROR:
        fprintf(stderr, "%s\n", result.diagnostic);
        return EC_IOERR;

    default:
        fprintf(stderr, "%s\n", result.diagnostic);
        return EC_DATA;
    }
}

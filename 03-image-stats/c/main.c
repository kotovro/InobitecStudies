#include "ppm_stats.h"

#include "../../common/c/exit_codes.h"
#include "../../common/c/version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage(void) {
    printf("Использование: image_stats\n");
    printf("\n");
    printf("Читает PPM P3 из stdin до EOF, выводит статистику:\n");
    printf("  размеры, число пикселей, средний цвет, яркость, гистограмма\n");
    printf("\n");
    printf("Работает в конвейере: gen_image | image_stats\n");
    printf("\n");
    printf("Опции:\n");
    printf("  --help      показать справку\n");
    printf("  --version   показать версию\n");
}

void print_version(void) { printf("image_stats %s\n", KV_VERSION); }

int main(int argc, char** argv) {
    if (argc >= 2 && strcmp(argv[1], "--help") == 0) {
        print_usage();
        return EC_OK;
    }

    if (argc >= 2 && strcmp(argv[1], "--version") == 0) {
        print_version();
        return EC_OK;
    }

    struct PpmResult result = ppm_read(stdin);
    if (result.error != PRE_OK) {
        fprintf(stderr, "%s\n", result.diagnostic);
        if (result.error == PRE_EMPTY_INPUT)
            return EC_NOINPUT;
        if (result.error == PRE_IO_ERROR)
            return EC_IOERR;
        return EC_DATA;
    }

    struct Stats stats;
    compute_stats(&result.image, &stats);
    ppm_print_stats(&stats, stdout);

    ppm_image_free(&result.image);
    return EC_OK;
}
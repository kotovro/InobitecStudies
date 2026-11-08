#include "ppm_stats.h"

#include "../../common/c/exit_codes.h"
#include "../../common/c/version.h"

#include <stdio.h>
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

    return run_stats(stdin, stdout);
}
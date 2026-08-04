#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../common/c/exit_codes.h"
#include "../../common/c/version.h"
#include "pixel_word.h"
#include "read_passport.h"

void print_usage(void) {
    printf("Использование: read_passport\n");
    printf("\n");
    printf("Интерактивно читает из stdin название изображения и число\n");
    printf("пикселей, выводит фразу со склонением «пиксель/пикселя/пикселей».\n");
    printf("\n");
    printf("Опции:\n");
    printf("  --help      показать справку\n");
    printf("  --version   показать версию\n");
}

void print_version(void) { printf("read_passport %s\n", KV_VERSION); }

int main(int argc, char** argv) {
    setlocale(LC_ALL, "Russian_Russia.1251");

    if (argc >= 2 && strcmp(argv[1], "--help") == 0) {
        print_usage();
        return EC_OK;
    }

    if (argc >= 2 && strcmp(argv[1], "--version") == 0) {
        print_version();
        return EC_OK;
    }

    struct PassportResult result = read_passport();
    if (result.error != PE_OK) {
        switch (result.error) {
        case PE_NO_INPUT:
            return EC_NOINPUT;
        case PE_EMPTY_NAME:
        case PE_BAD_COUNT:
        case PE_NEGATIVE_COUNT:
            return EC_DATA;
        case PE_IO_ERROR:
            return EC_IOERR;
        default:
            return 1;
        }
    }

    printf("Изображение \xAB%s\xBB: %d %s.\n", result.name, result.count, pixel_word(result.count));
    free(result.name);
    return EC_OK;
}
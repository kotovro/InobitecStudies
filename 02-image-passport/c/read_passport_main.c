#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../common/c/exit_codes.h"
#include "pixel_word.h"
#include "read_passport.h"

int main(void) {
    setlocale(LC_ALL, "Russian_Russia.1251");

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

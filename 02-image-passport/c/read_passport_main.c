#include "pixel_word.h"
#include "read_passport.h"

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    setlocale(LC_ALL, "Russian_Russia.1251");

    struct PassportResult result = read_passport();
    if (result.error != PE_OK) {
        switch (result.error) {
        case PE_NO_INPUT:
            return 66;
        case PE_EMPTY_NAME:
        case PE_BAD_COUNT:
        case PE_NEGATIVE_COUNT:
            return 65;
        case PE_IO_ERROR:
            return 74;
        default:
            return 1;
        }
    }

    printf("Изображение «%s»: %d %s.\n", result.name, result.count, pixel_word(result.count));
    free(result.name);
    return 0;
}

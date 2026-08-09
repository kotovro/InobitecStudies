#include "read_passport.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void safe_strerror(int errnum, char* buf, size_t bufsz) {
#ifdef _WIN32
    strerror_s(buf, bufsz, errnum);
#else
    strerror_r(errnum, buf, bufsz); // POSIX версия
#endif
}

struct PassportResult read_passport(void) {
    printf("Введите название изображения: \n");

    char name_buf[1024];
    if (!fgets(name_buf, sizeof(name_buf), stdin)) {
        if (feof(stdin)) {
            fprintf(stderr, "нет входных данных\n");
            return (struct PassportResult){PE_NO_INPUT, NULL, 0};
        }
        char errbuf[256];
        safe_strerror(errno, errbuf, sizeof errbuf);
        fprintf(stderr, "сбой ввода: %s (errno %d)\n", errbuf, errno);
        return (struct PassportResult){PE_IO_ERROR, NULL, 0};
    }

    size_t name_len = strlen(name_buf);
    if (name_len > 0 && name_buf[name_len - 1] == '\n')
        name_buf[--name_len] = '\0';

    while (name_len > 0 && (name_buf[name_len - 1] == ' ' || name_buf[name_len - 1] == '\t'))
        name_buf[--name_len] = '\0';
    size_t trim_start = 0;
    while (name_buf[trim_start] == ' ' || name_buf[trim_start] == '\t')
        trim_start++;
    if (trim_start > 0) {
        name_len -= trim_start;
        memmove(name_buf, name_buf + trim_start, name_len + 1);
    }

    if (name_len == 0) {
        fprintf(stderr, "название не может быть пустым\n");
        return (struct PassportResult){PE_EMPTY_NAME, NULL, 0};
    }

    char* name = (char*)malloc(name_len + 1);
    if (!name) {
        fprintf(stderr, "не удалось выделить память\n");
        return (struct PassportResult){PE_IO_ERROR, NULL, 0};
    }
    memcpy(name, name_buf, name_len + 1);

    printf("Введите количество пикселей: \n");

    char count_str[64];
    if (!fgets(count_str, sizeof(count_str), stdin)) {
        if (feof(stdin))
            fprintf(stderr, "нет входных данных\n");
        else {
            char errbuf[256];
            safe_strerror(errno, errbuf, sizeof errbuf);
            fprintf(stderr, "сбой ввода: %s (errno %d)\n", errbuf, errno);
        }
        free(name);
        return (struct PassportResult){feof(stdin) ? PE_NO_INPUT : PE_IO_ERROR, NULL, 0};
    }

    size_t count_len = strlen(count_str);
    if (count_len > 0 && count_str[count_len - 1] == '\n')
        count_str[--count_len] = '\0';

    char* end = NULL;
    errno = 0;
    long count = strtol(count_str, &end, 10);
    if (end == count_str || *end != '\0') {
        fprintf(stderr, "количество пикселей должно быть целым числом; получено: %s\n", count_str);
        free(name);
        return (struct PassportResult){PE_BAD_COUNT, NULL, 0};
    }
    if (errno == ERANGE) {
        fprintf(stderr, "количество пикселей должно быть целым числом; получено: %s\n", count_str);
        free(name);
        return (struct PassportResult){PE_BAD_COUNT, NULL, 0};
    }
    if (count <= 0) {
        fprintf(stderr, "количество пикселей должно быть положительным; получено: %s\n", count_str);
        free(name);
        return (struct PassportResult){PE_NEGATIVE_COUNT, NULL, 0};
    }

    return (struct PassportResult){PE_OK, name, (int32_t)count};
}
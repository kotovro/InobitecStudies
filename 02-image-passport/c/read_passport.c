#include "read_passport.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../common/c/strerror.h"

static void trim_whitespace(char* s) {
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1]))
        s[--len] = '\0';

    size_t start = 0;
    while (s[start] != '\0' && isspace((unsigned char)s[start]))
        ++start;
    if (start > 0)
        memmove(s, s + start, len - start + 1);
}

struct PassportResult read_passport(FILE* in) {
    printf("Введите название изображения: \n");

    char name_buf[1024];
    if (!fgets(name_buf, sizeof(name_buf), in)) {
        if (feof(in))
            return (struct PassportResult){.error = PE_NO_INPUT};
        return (struct PassportResult){.error = PE_IO_ERROR, .system_errno = errno};
    }

    trim_whitespace(name_buf);
    size_t name_len = strlen(name_buf);
    if (name_len == 0)
        return (struct PassportResult){.error = PE_EMPTY_NAME};

    char* name = (char*)malloc(name_len + 1);
    if (!name)
        return (struct PassportResult){.error = PE_IO_ERROR, .system_errno = errno};
    memcpy(name, name_buf, name_len + 1);

    printf("Введите количество пикселей: \n");

    char count_str[64];
    if (!fgets(count_str, sizeof(count_str), in)) {
        free(name);
        if (feof(in))
            return (struct PassportResult){.error = PE_NO_INPUT};
        return (struct PassportResult){.error = PE_IO_ERROR, .system_errno = errno};
    }

    trim_whitespace(count_str);

    char* end = NULL;
    errno = 0;
    long count = strtol(count_str, &end, 10);
    if (end == count_str || *end != '\0' || errno == ERANGE) {
        struct PassportResult result = {.error = PE_BAD_COUNT};
        snprintf(result.bad_value, sizeof result.bad_value, "%s", count_str);
        free(name);
        return result;
    }
    if (count <= 0) {
        struct PassportResult result = {.error = PE_NEGATIVE_COUNT};
        snprintf(result.bad_value, sizeof result.bad_value, "%s", count_str);
        free(name);
        return result;
    }

    return (struct PassportResult){.error = PE_OK, .name = name, .count = (int32_t)count};
}

void passport_error_message(const struct PassportResult* result, char* buf, size_t bufsz) {
    switch (result->error) {
    case PE_NO_INPUT:
        snprintf(buf, bufsz, "Нет ввода");
        break;
    case PE_EMPTY_NAME:
        snprintf(buf, bufsz, "Название изображения не может быть пустым");
        break;
    case PE_BAD_COUNT:
        snprintf(buf, bufsz, "количество пикселей должно быть числом; получено: %s",
                 result->bad_value);
        break;
    case PE_NEGATIVE_COUNT:
        snprintf(buf, bufsz, "количество пикселей должно быть положительным; получено: %s",
                 result->bad_value);
        break;
    case PE_IO_ERROR: {
        char errbuf[256];
        safe_strerror(result->system_errno, errbuf, sizeof errbuf);
        snprintf(buf, bufsz, "Сбой ввода: %s (errno %d)", errbuf, result->system_errno);
        break;
    }
    default:
        buf[0] = '\0';
        break;
    }
}

#ifndef KV_READ_PASSPORT_H
#define KV_READ_PASSPORT_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum PassportError {
    PE_OK,
    PE_NO_INPUT,
    PE_EMPTY_NAME,
    PE_BAD_COUNT,
    PE_NEGATIVE_COUNT,
    PE_IO_ERROR
};

enum { PASSPORT_BAD_VALUE_SIZE = 64 };

struct PassportResult {
    enum PassportError error;
    char* name;
    int32_t count;
    char bad_value[PASSPORT_BAD_VALUE_SIZE];
    int system_errno;
};

struct PassportResult read_passport(FILE* in);

void passport_error_message(const struct PassportResult* result, char* buf, size_t bufsz);

#endif

#ifndef KV_READ_PASSPORT_H
#define KV_READ_PASSPORT_H

#include <stdint.h>

enum PassportError {
    PE_OK,
    PE_NO_INPUT,
    PE_EMPTY_NAME,
    PE_BAD_COUNT,
    PE_NEGATIVE_COUNT,
    PE_IO_ERROR
};

struct PassportResult {
    enum PassportError error;
    char* name;
    int32_t count;
};

struct PassportResult read_passport(void);

#endif

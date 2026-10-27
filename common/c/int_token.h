#ifndef KV_INT_TOKEN_H
#define KV_INT_TOKEN_H

#include <stdint.h>

// Строгий разбор целого: семантика std::from_chars (C++23).
// Допустимо: необязательный '-' (только для знаковых), хотя бы одна десятичная
// цифра, ведущие нули. Недопустимо: '+', пробельные символы, хвост после числа,
// пустая строка - это KV_INT_INVALID. Значение вне целевого типа - KV_INT_RANGE.
enum KvIntParse {
    KV_INT_OK,
    KV_INT_INVALID,
    KV_INT_RANGE,
};

static inline enum KvIntParse kv_parse_i32(const char* s, int32_t* out) {
    if (!s || *s == '\0')
        return KV_INT_INVALID;

    int negative = 0;
    const char* p = s;
    if (*p == '-') {
        negative = 1;
        ++p;
    }

    int64_t limit = negative ? -(int64_t)INT32_MIN : (int64_t)INT32_MAX;
    int64_t value = 0;
    for (; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9')
            return KV_INT_INVALID;
        int64_t digit = *p - '0';
        if (value > (limit - digit) / 10)
            return KV_INT_RANGE;
        value = value * 10 + digit;
    }
    if (p == s + negative)
        return KV_INT_INVALID;

    *out = (int32_t)(negative ? -value : value);
    return KV_INT_OK;
}

static inline enum KvIntParse kv_parse_u32(const char* s, uint32_t* out) {
    if (!s || *s == '\0')
        return KV_INT_INVALID;

    uint64_t value = 0;
    for (const char* p = s; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9')
            return KV_INT_INVALID;
        uint64_t digit = (uint64_t)(*p - '0');
        if (value > ((uint64_t)UINT32_MAX - digit) / 10)
            return KV_INT_RANGE;
        value = value * 10 + digit;
    }

    *out = (uint32_t)value;
    return KV_INT_OK;
}

#endif

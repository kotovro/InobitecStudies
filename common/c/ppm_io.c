#include "ppm_io.h"

#include <errno.h>
#include <stdlib.h>

#include "strerror.h"

// -------------------------------------------------------------------
// Helper: skip whitespace and optional #-comments.
// allow_hash: 1 = skip #-lines (header phase), 0 = return '#' to caller
// Returns first non-whitespace char, or EOF.
static int skip_field(FILE* f, int* line_num, int allow_hash) {
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == ' ' || c == '\t' || c == '\r')
            continue;
        if (c == '\n') {
            ++*line_num;
            continue;
        }
        if (c == '#') {
            if (!allow_hash)
                return c;
            while ((c = fgetc(f)) != EOF && c != '\n')
                ;
            if (c == '\n')
                ++*line_num;
            continue;
        }
        return c;
    }
    return EOF;
}

enum TokenResult {
    TOKEN_OK,
    TOKEN_EOF,
    TOKEN_ERROR,
};

// Helper: set the "unexpected end of file" diagnostic for a header field.
static void set_eof_error(struct PpmResult* result, int line_num) {
    result->error = PRE_BAD_NUMBER;
    result->error_line = line_num;
    snprintf(result->diagnostic, sizeof(result->diagnostic), "строка %d: неожиданный конец файла",
             line_num);
}

// Helper: read a full integer token, skipping whitespace/comments.
// allow_hash: 1 = skip #-lines (header phase), 0 = treat '#' as an error.
// Returns TOKEN_OK (*val set, may be negative), TOKEN_EOF, or TOKEN_ERROR
// (result->diagnostic set) for fractional/non-numeric/overflow/IO errors.
static enum TokenResult read_int_token(FILE* f, int* line_num, int allow_hash, long long* val,
                                       struct PpmResult* result) {
    int c = skip_field(f, line_num, allow_hash);
    if (c == EOF) {
        if (ferror(f)) {
            int e = errno;
            char buf[256];
            safe_strerror(e, buf, sizeof buf);
            result->error = PRE_IO_ERROR;
            result->error_line = *line_num;
            snprintf(result->diagnostic, sizeof(result->diagnostic), "сбой чтения: %s (errno %d)",
                     buf, e);
            return TOKEN_ERROR;
        }
        return TOKEN_EOF;
    }

    if (c == '#') {
        result->error = PRE_BAD_NUMBER;
        result->error_line = *line_num;
        snprintf(result->diagnostic, sizeof(result->diagnostic),
                 "строка %d: символ '#' не допускается в данных", *line_num);
        return TOKEN_ERROR;
    }

    char token[128];
    size_t token_len = 0;
    int has_dot = 0;
    int has_digit = 0;
    int invalid = 0;
    int negative = 0;
    int overflow = 0;
    long long magnitude = 0;

    for (;;) {
        if (c == EOF || c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '#')
            break;

        if (token_len < sizeof(token) - 1)
            token[token_len] = (char)c;
        ++token_len;

        if (c == '.') {
            has_dot = 1;
        } else if ((c == '-' || c == '+') && token_len == 1) {
            negative = (c == '-');
            if (c == '+')
                invalid = 1;
        } else if (c >= '0' && c <= '9') {
            has_digit = 1;
            if (!overflow) {
                magnitude = magnitude * 10 + (c - '0');
                if (magnitude > 0x7FFFFFFF)
                    overflow = 1;
            }
        } else {
            invalid = 1;
        }

        c = fgetc(f);
    }
    if (c != EOF)
        ungetc(c, f);

    token[token_len < sizeof(token) ? token_len : sizeof(token) - 1] = '\0';

    if (has_dot) {
        result->error = PRE_BAD_NUMBER;
        result->error_line = *line_num;
        snprintf(result->diagnostic, sizeof(result->diagnostic),
                 "строка %d: значение должно быть целым числом; получено: %s", *line_num, token);
        return TOKEN_ERROR;
    }

    if (invalid || !has_digit) {
        result->error = PRE_BAD_NUMBER;
        result->error_line = *line_num;
        if (token_len == 1 && (unsigned char)token[0] < ' ')
            snprintf(result->diagnostic, sizeof(result->diagnostic),
                     "строка %d: нечисловое значение, код 0x%02x", *line_num,
                     (unsigned char)token[0]);
        else
            snprintf(result->diagnostic, sizeof(result->diagnostic),
                     "строка %d: нечисловое значение, получено: %s", *line_num, token);
        return TOKEN_ERROR;
    }

    if (overflow) {
        result->error = PRE_BAD_NUMBER;
        result->error_line = *line_num;
        snprintf(result->diagnostic, sizeof(result->diagnostic),
                 "строка %d: число превышает допустимый диапазон", *line_num);
        return TOKEN_ERROR;
    }

    *val = negative ? -magnitude : magnitude;
    return TOKEN_OK;
}

// -------------------------------------------------------------------
// ppm_read
// -------------------------------------------------------------------

struct PpmResult ppm_read(FILE* f) {
    struct PpmResult result = {PRE_OK, 0, "", {0, 0, 0, NULL}};
    int line_num = 1;

    // ---- 1. Magic "P3" ----
    int c = skip_field(f, &line_num, 1);
    if (c == EOF) {
        if (ferror(f)) {
            int e = errno;
            char buf[256];
            safe_strerror(e, buf, sizeof buf);
            result.error = PRE_IO_ERROR;
            result.error_line = line_num;
            snprintf(result.diagnostic, sizeof(result.diagnostic), "сбой чтения: %s (errno %d)",
                     buf, e);
            return result;
        }
        result.error = PRE_EMPTY_INPUT;
        result.error_line = line_num;
        snprintf(result.diagnostic, sizeof(result.diagnostic), "нет входных данных");
        return result;
    }

    if (c != 'P') {
        result.error = PRE_BAD_MAGIC;
        result.error_line = line_num;
        if (c >= ' ' && c <= '~')
            snprintf(result.diagnostic, sizeof(result.diagnostic),
                     "строка %d: ожидалось 'P3', получено: '%c'", line_num, (char)c);
        else
            snprintf(result.diagnostic, sizeof(result.diagnostic),
                     "строка %d: ожидалось 'P3', код 0x%02x", line_num, (unsigned char)c);
        return result;
    }

    c = fgetc(f);
    if (c == EOF) {
        result.error = PRE_BAD_MAGIC;
        result.error_line = line_num;
        snprintf(result.diagnostic, sizeof(result.diagnostic),
                 "строка %d: ожидалось 'P3', получено: 'P'", line_num);
        return result;
    }
    if (c != '3') {
        result.error = PRE_BAD_MAGIC;
        result.error_line = line_num;
        if (c >= ' ' && c <= '~')
            snprintf(result.diagnostic, sizeof(result.diagnostic),
                     "строка %d: ожидалось 'P3', получено: 'P%c'", line_num, (char)c);
        else
            snprintf(result.diagnostic, sizeof(result.diagnostic),
                     "строка %d: ожидалось 'P3', код 0x%02x после 'P'", line_num, (unsigned char)c);
        return result;
    }

    // ---- 2. Width, Height, Maxval ----
    long long w_val = 0;
    enum TokenResult wr = read_int_token(f, &line_num, 1, &w_val, &result);
    if (wr == TOKEN_EOF) {
        set_eof_error(&result, line_num);
        return result;
    }
    if (wr == TOKEN_ERROR)
        return result;
    if (w_val <= 0) {
        result.error = PRE_BAD_NUMBER;
        result.error_line = line_num;
        snprintf(result.diagnostic, sizeof(result.diagnostic),
                 "строка %d: ширина должна быть положительным числом; получено: %lld", line_num,
                 w_val);
        return result;
    }

    long long h_val = 0;
    enum TokenResult hr = read_int_token(f, &line_num, 1, &h_val, &result);
    if (hr == TOKEN_EOF) {
        set_eof_error(&result, line_num);
        return result;
    }
    if (hr == TOKEN_ERROR)
        return result;
    if (h_val <= 0) {
        result.error = PRE_BAD_NUMBER;
        result.error_line = line_num;
        snprintf(result.diagnostic, sizeof(result.diagnostic),
                 "строка %d: высота должна быть положительным числом; получено: %lld", line_num,
                 h_val);
        return result;
    }

    long long m_val = 0;
    enum TokenResult mr = read_int_token(f, &line_num, 1, &m_val, &result);
    if (mr == TOKEN_EOF) {
        set_eof_error(&result, line_num);
        return result;
    }
    if (mr == TOKEN_ERROR)
        return result;
    if (m_val != 255) {
        result.error = PRE_BAD_NUMBER;
        result.error_line = line_num;
        snprintf(result.diagnostic, sizeof(result.diagnostic),
                 "строка %d: максимальное значение канала должно быть 255; получено: %lld",
                 line_num, m_val);
        return result;
    }

    result.image.width = (int32_t)w_val;
    result.image.height = (int32_t)h_val;
    result.image.max_val = (uint16_t)m_val;

    long long total_pixels = w_val * h_val;
    if (total_pixels == 0) {
        result.error = PRE_BAD_NUMBER;
        result.error_line = line_num;
        snprintf(result.diagnostic, sizeof(result.diagnostic),
                 "строка %d: изображение не содержит пикселей", line_num);
        return result;
    }

    result.image.pixels = (struct Pixel*)malloc((size_t)total_pixels * sizeof(struct Pixel));
    if (!result.image.pixels) {
        result.error = PRE_ALLOC_ERROR;
        result.error_line = line_num;
        snprintf(result.diagnostic, sizeof(result.diagnostic),
                 "не удалось выделить память для %lld пикселей", total_pixels);
        return result;
    }

    // ---- 3. Pixel data ----
    int pixel_error = 0;
    int pixel_eof = 0;
    long long pixel_count = 0;

    for (long long i = 0; i < total_pixels; ++i) {
        long long r_val, g_val, b_val;
        enum TokenResult tr;

        tr = read_int_token(f, &line_num, 0, &r_val, &result);
        if (tr != TOKEN_OK) {
            pixel_error = 1;
            pixel_eof = (tr == TOKEN_EOF);
            break;
        }
        tr = read_int_token(f, &line_num, 0, &g_val, &result);
        if (tr != TOKEN_OK) {
            pixel_error = 1;
            pixel_eof = (tr == TOKEN_EOF);
            break;
        }
        tr = read_int_token(f, &line_num, 0, &b_val, &result);
        if (tr != TOKEN_OK) {
            pixel_error = 1;
            pixel_eof = (tr == TOKEN_EOF);
            break;
        }

        if (r_val < 0 || r_val > result.image.max_val || g_val < 0 ||
            g_val > result.image.max_val || b_val < 0 || b_val > result.image.max_val) {
            result.error = PRE_CHANNEL_RANGE;
            result.error_line = line_num;
            snprintf(result.diagnostic, sizeof(result.diagnostic),
                     "строка %d: значение канала должно быть в [0; %d]; получено: %lld %lld %lld",
                     line_num, result.image.max_val, r_val, g_val, b_val);
            free(result.image.pixels);
            result.image.pixels = NULL;
            return result;
        }

        result.image.pixels[i].r = (uint8_t)r_val;
        result.image.pixels[i].g = (uint8_t)g_val;
        result.image.pixels[i].b = (uint8_t)b_val;
        ++pixel_count;
    }

    if (pixel_error) {
        if (pixel_eof && pixel_count < total_pixels) {
            result.error = PRE_TOO_FEW_PIXELS;
            result.error_line = line_num;
            snprintf(result.diagnostic, sizeof(result.diagnostic),
                     "строка %d: получено только %lld пикселей (ожидалось %lld)", line_num,
                     pixel_count, total_pixels);
        }
        free(result.image.pixels);
        result.image.pixels = NULL;
        return result;
    }

    // ---- 4. Check for trailing data ----
    while ((c = fgetc(f)) != EOF) {
        if (c == ' ' || c == '\t' || c == '\r')
            continue;
        if (c == '\n') {
            ++line_num;
            continue;
        }
        if (c == '#') {
            result.error = PRE_BAD_NUMBER;
            result.error_line = line_num;
            snprintf(result.diagnostic, sizeof(result.diagnostic),
                     "строка %d: символ '#' не допускается в данных", line_num);
            free(result.image.pixels);
            result.image.pixels = NULL;
            return result;
        }
        result.error = PRE_TOO_MANY_PIXELS;
        result.error_line = line_num;
        snprintf(result.diagnostic, sizeof(result.diagnostic),
                 "строка %d: лишние данные после %lld пикселей", line_num, pixel_count);
        free(result.image.pixels);
        result.image.pixels = NULL;
        return result;
    }

    if (ferror(f)) {
        int e = errno;
        char buf[256];
        safe_strerror(e, buf, sizeof buf);
        result.error = PRE_IO_ERROR;
        result.error_line = line_num;
        snprintf(result.diagnostic, sizeof(result.diagnostic), "сбой чтения: %s (errno %d)", buf,
                 e);
        free(result.image.pixels);
        result.image.pixels = NULL;
        return result;
    }

    return result;
}

void ppm_image_free(struct Image* img) {
    if (img) {
        free(img->pixels);
        img->pixels = NULL;
    }
}

// -------------------------------------------------------------------
// PpmWriter
// -------------------------------------------------------------------

void ppm_writer_init(struct PpmWriter* w, FILE* f, int32_t width, int32_t height) {
    w->f = f;
    w->width = width;
    w->col = 0;
    fprintf(f, "P3\n%d %d\n255\n", (int)width, (int)height);
}

void ppm_writer_put(struct PpmWriter* w, uint8_t r, uint8_t g, uint8_t b) {
    if (w->col == 0)
        fprintf(w->f, "%3d %3d %3d", (int)r, (int)g, (int)b);
    else
        fprintf(w->f, " %3d %3d %3d", (int)r, (int)g, (int)b);

    ++w->col;
    if (w->col >= w->width) {
        fprintf(w->f, "\n");
        w->col = 0;
    }
}

int ppm_writer_finish(struct PpmWriter* w) { return (fflush(w->f) != 0 || ferror(w->f)) ? -1 : 0; }
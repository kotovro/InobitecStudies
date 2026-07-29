#include "ppm_stats.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

// Внутренние утилиты парсинга

// skip_field: пропускает пробельные символы.
// Параметр allow_hash:
//   1 — в заголовке: строки, начинающиеся с '#', целиком пропускаются
//   0 — в данных:  '#' возвращается как обычный символ (вызывает ошибку у
//   caller'а)
//
// Возвращает: первый непробельный символ, либо EOF
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

// read_field: пропускает разделители, читает десятичное число.
// Возвращает: 0 при успехе (число в *value), -1 при ошибке
// (result->error и diagnostic заполнены)
static int read_field(FILE* f, int* line_num, int allow_hash, int64_t* value,
                      struct StatsResult* result) {
    int c = skip_field(f, line_num, allow_hash);
    if (c == EOF) {
        if (ferror(f)) {
            int e = errno;
            char errbuf[256];
            strerror_s(errbuf, sizeof errbuf, e);
            result->error = SE_IO_ERROR;
            snprintf(result->diagnostic, sizeof(result->diagnostic), "сбой ввода: %s (errno %d)",
                     errbuf, e);
            return -1;
        }
        result->error = SE_BAD_NUMBER;
        snprintf(result->diagnostic, sizeof(result->diagnostic),
                 "строка %d: неожиданный конец файла", *line_num);
        return -1;
    }

    if (c == '#') {
        result->error = SE_BAD_NUMBER;
        snprintf(result->diagnostic, sizeof(result->diagnostic),
                 "строка %d: символ '#' не допускается в данных", *line_num);
        return -1;
    }

    if (c < '0' || c > '9') {
        result->error = SE_BAD_NUMBER;
        if (c >= ' ' && c <= '~')
            snprintf(result->diagnostic, sizeof(result->diagnostic),
                     "строка %d: ожидалось число, получено: '%c'", *line_num, (char)c);
        else
            snprintf(result->diagnostic, sizeof(result->diagnostic),
                     "строка %d: ожидалось число, получен код 0x%02x", *line_num, (unsigned char)c);
        return -1;
    }

    int64_t val = 0;
    while (c >= '0' && c <= '9') {
        val = val * 10 + (c - '0');
        if (val > 0x7FFFFFFF) {
            result->error = SE_BAD_NUMBER;
            snprintf(result->diagnostic, sizeof(result->diagnostic),
                     "строка %d: число превышает допустимый диапазон", *line_num);
            return -1;
        }
        c = fgetc(f);
        if (c == EOF)
            break;
    }
    if (c != EOF)
        ungetc(c, f);

    *value = val;
    return 0;
}

// Утилиты

double luma(int32_t r, int32_t g, int32_t b) { return 0.299 * r + 0.587 * g + 0.114 * b; }

// Публичные функции

struct StatsResult ppm_read_stats(FILE* f) {
    struct StatsResult result = {SE_OK, "", {0, 0, 0, 0, 0, 0, 0, 0.0, 0.0, {0}}};
    int line_num = 1;

    // ---- 1. Magic: "P3" ----
    int c = skip_field(f, &line_num, 1);
    if (c == EOF) {
        if (ferror(f)) {
            int e = errno;
            char errbuf[256];
            strerror_s(errbuf, sizeof errbuf, e);
            result.error = SE_IO_ERROR;
            snprintf(result.diagnostic, sizeof(result.diagnostic), "сбой ввода: %s (errno %d)",
                     errbuf, e);
            return result;
        }
        result.error = SE_EMPTY_INPUT;
        snprintf(result.diagnostic, sizeof(result.diagnostic), "нет входных данных");
        return result;
    }

    if (c != 'P') {
        result.error = SE_BAD_MAGIC;
        if (c >= ' ' && c <= '~')
            snprintf(result.diagnostic, sizeof(result.diagnostic),
                     "строка %d: ожидалось 'P3', получено: '%c'", line_num, (char)c);
        else
            snprintf(result.diagnostic, sizeof(result.diagnostic),
                     "строка %d: ожидалось 'P3', получен код 0x%02x", line_num, (unsigned char)c);
        return result;
    }

    c = fgetc(f);
    if (c == EOF) {
        result.error = SE_BAD_MAGIC;
        snprintf(result.diagnostic, sizeof(result.diagnostic),
                 "строка %d: ожидалось 'P3', получено: 'P'", line_num);
        return result;
    }
    if (c != '3') {
        result.error = SE_BAD_MAGIC;
        if (c >= ' ' && c <= '~')
            snprintf(result.diagnostic, sizeof(result.diagnostic),
                     "строка %d: ожидалось 'P3', получено: 'P%c'", line_num, (char)c);
        else
            snprintf(result.diagnostic, sizeof(result.diagnostic),
                     "строка %d: ожидалось 'P3', получен код 0x%02x после 'P'", line_num,
                     (unsigned char)c);
        return result;
    }

    // 2. Width
    int64_t w_val = 0;
    if (read_field(f, &line_num, 1, &w_val, &result) != 0)
        return result;
    if (w_val <= 0 || w_val > 0x7FFFFFFF) {
        result.error = SE_BAD_NUMBER;
        snprintf(result.diagnostic, sizeof(result.diagnostic),
                 "строка %d: ширина должна быть положительным целым; получено: %lld", line_num,
                 w_val);
        return result;
    }
    result.stats.width = (int32_t)w_val;

    // 3. Height
    int64_t h_val = 0;
    if (read_field(f, &line_num, 1, &h_val, &result) != 0)
        return result;
    if (h_val <= 0 || h_val > 0x7FFFFFFF) {
        result.error = SE_BAD_NUMBER;
        snprintf(result.diagnostic, sizeof(result.diagnostic),
                 "строка %d: высота должна быть положительным целым; получено: %lld", line_num,
                 h_val);
        return result;
    }
    result.stats.height = (int32_t)h_val;

    // 4. Maxval
    int64_t max_val = 0;
    if (read_field(f, &line_num, 1, &max_val, &result) != 0)
        return result;
    if (max_val != 255) {
        result.error = SE_BAD_NUMBER;
        snprintf(result.diagnostic, sizeof(result.diagnostic),
                 "строка %d: максимальное значение канала должно быть 255; получено: %lld",
                 line_num, max_val);
        return result;
    }
    result.stats.max_val = (int32_t)max_val;

    // 5. Попиксельные данные (PH_DATA — '#' не допускается)
    int64_t total_pixels = (int64_t)result.stats.width * result.stats.height;
    int32_t first_pixel = 1;
    int32_t pixel_error = 0;

    for (int64_t i = 0; i < total_pixels; ++i) {
        int64_t r_val, g_val, b_val;

        if (read_field(f, &line_num, 0, &r_val, &result) != 0) {
            pixel_error = 1;
            break;
        }
        if (read_field(f, &line_num, 0, &g_val, &result) != 0) {
            pixel_error = 1;
            break;
        }
        if (read_field(f, &line_num, 0, &b_val, &result) != 0) {
            pixel_error = 1;
            break;
        }

        if (r_val < 0 || r_val > result.stats.max_val || g_val < 0 ||
            g_val > result.stats.max_val || b_val < 0 || b_val > result.stats.max_val) {
            result.error = SE_CHANNEL_RANGE;
            snprintf(result.diagnostic, sizeof(result.diagnostic),
                     "строка %d: значение канала должно быть в [0; %d]; получено: %lld %lld %lld",
                     line_num, result.stats.max_val, r_val, g_val, b_val);
            return result;
        }

        int32_t r = (int32_t)r_val;
        int32_t g = (int32_t)g_val;
        int32_t b = (int32_t)b_val;

        result.stats.total_r += r;
        result.stats.total_g += g;
        result.stats.total_b += b;

        double y = luma(r, g, b);

        if (first_pixel) {
            result.stats.y_min = y;
            result.stats.y_max = y;
            first_pixel = 0;
        } else {
            if (y < result.stats.y_min)
                result.stats.y_min = y;
            if (y > result.stats.y_max)
                result.stats.y_max = y;
        }

        int32_t bin = (int32_t)(y * 8.0 / (result.stats.max_val + 1));
        if (bin >= 8)
            bin = 7;
        result.stats.histogram[bin]++;

        result.stats.pixel_count++;
    }

    // После цикла: если читали с ошибкой, различаем EOF (не хватило пикселей)
    // от прочих ошибок
    if (pixel_error) {
        if (result.error == SE_BAD_NUMBER && result.stats.pixel_count < total_pixels && feof(f)) {
            result.error = SE_TOO_FEW_PIXELS;
            snprintf(result.diagnostic, sizeof(result.diagnostic),
                     "строка %d: неожиданный конец файла после %lld пикселей (ожидалось %lld)",
                     line_num, result.stats.pixel_count, total_pixels);
        }
        return result;
    }

    // 6. Проверка на лишние данные после W*H пикселей
    while ((c = fgetc(f)) != EOF) {
        if (c == ' ' || c == '\t' || c == '\r')
            continue;
        if (c == '\n') {
            ++line_num;
            continue;
        }
        result.error = SE_TOO_MANY_PIXELS;
        snprintf(result.diagnostic, sizeof(result.diagnostic),
                 "строка %d: лишние данные после %lld пикселей", line_num,
                 result.stats.pixel_count);
        return result;
    }

    // Проверяем, не случилась ли ошибка ввода
    if (ferror(f)) {
        int e = errno;
        char errbuf[256];
        strerror_s(errbuf, sizeof errbuf, e);
        result.error = SE_IO_ERROR;
        snprintf(result.diagnostic, sizeof(result.diagnostic), "сбой ввода: %s (errno %d)", errbuf,
                 e);
        return result;
    }

    return result;
}

void ppm_print_stats(const struct Stats* s, FILE* out) {
    int32_t avg_r = (int32_t)((double)s->total_r / s->pixel_count + 0.5);
    int32_t avg_g = (int32_t)((double)s->total_g / s->pixel_count + 0.5);
    int32_t avg_b = (int32_t)((double)s->total_b / s->pixel_count + 0.5);

    fprintf(out, "%dx%d\n", s->width, s->height);
    fprintf(out, "пикселей: %lld\n", s->pixel_count);
    fprintf(out, "средний цвет: %d %d %d\n", avg_r, avg_g, avg_b);
    fprintf(out, "мин. яркость: %.1f\n", s->y_min);
    fprintf(out, "макс. яркость: %.1f\n", s->y_max);
    fprintf(out, "гистограмма яркости:\n");

    for (int32_t i = 0; i < 8; ++i) {
        double bin_width = (double)(s->max_val + 1) / 8.0;
        int32_t lo = (int32_t)(i * bin_width);
        int32_t hi = (i == 7) ? s->max_val : (int32_t)((i + 1) * bin_width) - 1;
        fprintf(out, "  [%3d,%3d]: %d\n", lo, hi, s->histogram[i]);
    }
}

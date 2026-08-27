#include "parse_args.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../common/c/version.h"

static struct ParseResult parse_args_old(int argc, char** argv) {
    struct ParseResult result = {0, PATTERN_GRADIENT, PE_OK, 0, 0, PR_RUN};

    if (argc < 2) {
        result.error = PE_NOARG;
        return result;
    }

    char* end;
    errno = 0;
    long n = strtol(argv[1], &end, 10);
    if (errno == ERANGE || end == argv[1] || *end != '\0') {
        result.error = PE_BADNUMBER;
        return result;
    }
    result.size = (int32_t)n;

    const char* name = (argc >= 3) ? argv[2] : "gradient";
    if (strcmp(name, "gradient") == 0)
        result.pattern = PATTERN_GRADIENT;
    else if (strcmp(name, "checker") == 0)
        result.pattern = PATTERN_CHECKER;
    else if (strcmp(name, "radial") == 0)
        result.pattern = PATTERN_RADIAL;
    else {
        result.error = PE_BADPATTERN;
        return result;
    }

    return result;
}

static struct ParseResult parse_args_new(int argc, char** argv) {
    struct ParseResult result = {0, PATTERN_RANDOM, PE_OK, 0, 0, PR_RUN};

    // Expect: --size N [--seed S]
    if (argc < 3) {
        result.error = PE_NOARG;
        return result;
    }

    if (strcmp(argv[1], "--size") != 0) {
        result.error = PE_BADNUMBER;
        return result;
    }

    char* end;
    errno = 0;
    long n = strtol(argv[2], &end, 10);
    if (errno == ERANGE || end == argv[2] || *end != '\0') {
        result.error = PE_BADNUMBER;
        return result;
    }
    result.size = (int32_t)n;

    if (argc >= 5 && strcmp(argv[3], "--seed") == 0) {
        errno = 0;
        long s = strtol(argv[4], &end, 10);
        if (errno == ERANGE || end == argv[4] || *end != '\0' || s < 0 || s > UINT32_MAX) {
            result.error = PE_BADSEED;
            return result;
        }
        result.seed = (uint32_t)s;
        result.seed_provided = 1;
    }

    return result;
}

struct ParseResult parse_args(int argc, char** argv) {
    if (argc >= 2 && strcmp(argv[1], "--help") == 0) {
        struct ParseResult result = {0, PATTERN_GRADIENT, PE_OK, 0, 0, PR_HELP};
        return result;
    }

    if (argc >= 2 && strcmp(argv[1], "--version") == 0) {
        struct ParseResult result = {0, PATTERN_GRADIENT, PE_OK, 0, 0, PR_VERSION};
        return result;
    }

    if (argc >= 2 && strcmp(argv[1], "--size") == 0)
        return parse_args_new(argc, argv);
    return parse_args_old(argc, argv);
}

void print_usage(void) {
    printf("Использование: gen_image <N> [pattern]\n");
    printf("               gen_image --size N [--seed S]\n");
    printf("\n");
    printf("Позиционный режим:\n");
    printf("  N        сторона квадрата (1-512)\n");
    printf("  pattern  gradient | checker | radial (по умолчанию gradient)\n");
    printf("\n");
    printf("Массовая генерация:\n");
    printf("  --size N    сторона (без ограничения 512)\n");
    printf("  --seed S    seed ГПСЧ (по умолчанию 42)\n");
    printf("\n");
    printf("Опции:\n");
    printf("  --help      показать справку\n");
    printf("  --version   показать версию\n");
}

void print_version(void) { printf("gen_image %s\n", KV_VERSION); }
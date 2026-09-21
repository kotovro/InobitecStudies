#include <stdio.h>
#include <string.h>

struct CaseInput {
    const char* key;
    const char* name;
    const char* count;
};

static const struct CaseInput kCases[] = {
    {"basic", "морской закат", "1920"}, {"single_1", "тест", "1"},
    {"plural_2", "тест", "2"},          {"plural_3", "тест", "3"},
    {"plural_4", "тест", "4"},          {"plural_5", "тест", "5"},
    {"plural_10", "тест", "10"},        {"plural_11", "тест", "11"},
    {"plural_12", "тест", "12"},        {"plural_14", "тест", "14"},
    {"plural_20", "тест", "20"},        {"plural_21", "тест", "21"},
    {"plural_22", "тест", "22"},        {"plural_100", "тест", "100"},
    {"plural_101", "тест", "101"},      {"plural_111", "тест", "111"},
    {"plural_121", "тест", "121"},      {"empty_name", "", "1920"},
    {"no_input", NULL, NULL},           {"bad_count", "тест", "abc"},
    {"negative", "тест", "-5"},         {"zero", "тест", "0"},
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ref_passport_input <case>\n");
        return 64;
    }

    const char* tc = argv[1];
    size_t case_count = sizeof(kCases) / sizeof(kCases[0]);
    for (size_t i = 0; i < case_count; ++i) {
        if (strcmp(tc, kCases[i].key) == 0) {
            if (kCases[i].name)
                printf("%s\n", kCases[i].name);
            if (kCases[i].count)
                printf("%s\n", kCases[i].count);
            return 0;
        }
    }

    fprintf(stderr, "unknown case: %s\n", tc);
    return 64;
}

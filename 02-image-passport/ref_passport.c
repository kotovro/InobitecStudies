#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_prompt1(void) { printf("Введите название изображения: \n"); }

static void print_prompt2(void) { printf("Введите количество пикселей: \n"); }

static void print_result(const char* name, int count, const char* word) {
    printf("Изображение \xAB%s\xBB: %d %s.\n", name, count, word);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ref_passport <case>\n");
        return 64;
    }

    const char* tc = argv[1];

    if (strcmp(tc, "basic") == 0) {
        print_prompt1();
        print_prompt2();
        print_result("морской закат", 1920, "пикселей");
        return 0;
    }
    if (strcmp(tc, "single_1") == 0) {
        print_prompt1();
        print_prompt2();
        print_result("тест", 1, "пиксель");
        return 0;
    }
    if (strcmp(tc, "plural_2") == 0) {
        print_prompt1();
        print_prompt2();
        print_result("тест", 2, "пикселя");
        return 0;
    }
    if (strcmp(tc, "plural_3") == 0) {
        print_prompt1();
        print_prompt2();
        print_result("тест", 3, "пикселя");
        return 0;
    }
    if (strcmp(tc, "plural_4") == 0) {
        print_prompt1();
        print_prompt2();
        print_result("тест", 4, "пикселя");
        return 0;
    }
    if (strcmp(tc, "plural_5") == 0) {
        print_prompt1();
        print_prompt2();
        print_result("тест", 5, "пикселей");
        return 0;
    }
    if (strcmp(tc, "plural_10") == 0) {
        print_prompt1();
        print_prompt2();
        print_result("тест", 10, "пикселей");
        return 0;
    }
    if (strcmp(tc, "plural_11") == 0) {
        print_prompt1();
        print_prompt2();
        print_result("тест", 11, "пикселей");
        return 0;
    }
    if (strcmp(tc, "plural_12") == 0) {
        print_prompt1();
        print_prompt2();
        print_result("тест", 12, "пикселей");
        return 0;
    }
    if (strcmp(tc, "plural_14") == 0) {
        print_prompt1();
        print_prompt2();
        print_result("тест", 14, "пикселей");
        return 0;
    }
    if (strcmp(tc, "plural_20") == 0) {
        print_prompt1();
        print_prompt2();
        print_result("тест", 20, "пикселей");
        return 0;
    }
    if (strcmp(tc, "plural_21") == 0) {
        print_prompt1();
        print_prompt2();
        print_result("тест", 21, "пиксель");
        return 0;
    }
    if (strcmp(tc, "plural_22") == 0) {
        print_prompt1();
        print_prompt2();
        print_result("тест", 22, "пикселя");
        return 0;
    }
    if (strcmp(tc, "plural_100") == 0) {
        print_prompt1();
        print_prompt2();
        print_result("тест", 100, "пикселей");
        return 0;
    }
    if (strcmp(tc, "plural_101") == 0) {
        print_prompt1();
        print_prompt2();
        print_result("тест", 101, "пиксель");
        return 0;
    }
    if (strcmp(tc, "plural_111") == 0) {
        print_prompt1();
        print_prompt2();
        print_result("тест", 111, "пикселей");
        return 0;
    }
    if (strcmp(tc, "plural_121") == 0) {
        print_prompt1();
        print_prompt2();
        print_result("тест", 121, "пиксель");
        return 0;
    }
    if (strcmp(tc, "empty_name") == 0) {
        print_prompt1();
        fprintf(stderr, "Название изображения не может быть пустым\n");
        return 65;
    }
    if (strcmp(tc, "no_input") == 0) {
        print_prompt1();
        fprintf(stderr, "Нет ввода\n");
        return 66;
    }
    if (strcmp(tc, "bad_count") == 0) {
        print_prompt1();
        print_prompt2();
        fprintf(stderr, "количество пикселей должно быть числом; получено: abc\n");
        return 65;
    }
    if (strcmp(tc, "negative") == 0) {
        print_prompt1();
        print_prompt2();
        fprintf(stderr, "количество пикселей должно быть положительным; получено: -5\n");
        return 65;
    }
    if (strcmp(tc, "zero") == 0) {
        print_prompt1();
        print_prompt2();
        fprintf(stderr, "количество пикселей должно быть положительным; получено: 0\n");
        return 65;
    }

    fprintf(stderr, "unknown case: %s\n", tc);
    return 64;
}

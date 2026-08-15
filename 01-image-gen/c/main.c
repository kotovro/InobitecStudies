#include "hsv_to_rgb.h"
#include "parse_args.h"
#include "patterns.h"

#include "../../common/c/ppm_io.h"

static void generate_ppm(const struct ParseResult* args) {
    struct PpmWriter writer;
    ppm_writer_init(&writer, stdout, args->size, args->size);
    uint32_t state = args->seed;
    for (int32_t y = 0; y < args->size; ++y)
        for (int32_t x = 0; x < args->size; ++x) {
            struct RGB c = {0};
            switch (args->pattern) {
            case PATTERN_GRADIENT:
                c = gradient_pixel(x, y, args->size);
                break;
            case PATTERN_CHECKER:
                c = checker_pixel(x, y, args->size);
                break;
            case PATTERN_RADIAL:
                c = radial_pixel(x, y, args->size);
                break;
            case PATTERN_RANDOM:
                c = random_pixel(&state);
                break;
            }
            ppm_writer_put(&writer, c.r, c.g, c.b);
        }
}

int main(int argc, char** argv) {
    struct ParseResult args = parse_args(argc, argv);

    if (args.request == PR_HELP) {
        print_usage();
        return EC_OK;
    }

    if (args.request == PR_VERSION) {
        print_version();
        return EC_OK;
    }

    if (args.error != PE_OK) {
        switch (args.error) {
        case PE_NOARG:
            fprintf(stderr, "N не указано. Использование: gen_image <N> [pattern]\n");
            return EC_USAGE;
        case PE_BADNUMBER:
            fprintf(stderr, "N должно быть целым числом; получено: %s\n", argc >= 2 ? argv[1] : "");
            return EC_USAGE;
        case PE_BADPATTERN:
            fprintf(stderr, "Неизвестный паттерн: %s. Допустимые: gradient, checker, radial\n",
                    argc >= 3 ? argv[2] : "");
            return EC_USAGE;
        }
    }

    if (args.size < 1) {
        fprintf(stderr, "размер должен быть положительным; получено: %d\n", args.size);
        return EC_USAGE;
    }

    if (args.pattern != PATTERN_RANDOM && args.size > 512) {
        fprintf(stderr, "N должно быть в [1; 512]; получено: %d\n", args.size);
        return EC_USAGE;
    }

    generate_ppm(&args);
    return EC_OK;
}
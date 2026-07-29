#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    if (argc < 2)
        return 1;
    int size = atoi(argv[1]);
    if (size < 1)
        return 1;

    printf("P3\n%d %d\n255\n", size, size);
    int max_coord = (size == 1) ? 1 : (size - 1);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            int r = x * 255 / max_coord;
            int g = (max_coord - y) * 255 / max_coord;
            int b = 0;
            printf("%3d %3d %3d", r, g, b);
            if (x + 1 < size)
                printf(" ");
        }
        printf("\n");
    }
    return 0;
}

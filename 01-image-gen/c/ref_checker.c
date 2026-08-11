#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    if (argc < 2)
        return 1;
    int size = atoi(argv[1]);
    if (size < 1)
        return 1;

    printf("P3\n%d %d\n255\n", size, size);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            int v = ((x + y) % 2 == 0) ? 255 : 0;
            printf("%3d %3d %3d", v, v, v);
            if (x + 1 < size)
                printf(" ");
        }
        printf("\n");
    }
    return 0;
}
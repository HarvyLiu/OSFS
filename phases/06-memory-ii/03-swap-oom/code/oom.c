// oom.c -- capped allocate-and-touch, honest NULL. Lesson docs/en.md.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    long mb = argc > 1 ? atol(argv[1]) : 3;
    if (mb < 0 || mb > 256) { fprintf(stderr, "cap 0..256\n"); return 2; }
    char *blocks[256];
    long got = 0;
    for (long i = 0; i < mb; i++) {
        blocks[i] = malloc(1 << 20);
        if (!blocks[i]) break;
        memset(blocks[i], 0xA5, 1 << 20);
        got++;
    }
    printf("allocated=%ldMB requested=%ldMB\n", got, mb);
    for (long i = 0; i < got; i++) free(blocks[i]);
    return 0;
}

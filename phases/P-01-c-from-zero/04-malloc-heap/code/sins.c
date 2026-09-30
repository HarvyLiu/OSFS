// sins.c -- one heap sin per argv. Build with ASan. Lesson docs/en.md.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int do_leak(void) {
    int *p = malloc(64);
    if (!p) return 1;
    p[0] = 1;
    printf("leaked 64 bytes (no free)\n");
    return 0;
}

static int do_doublefree(void) {
    int *p = malloc(64);
    if (!p) return 1;
    free(p);
    free(p);  // BUG on purpose: heap metadata corrupt
    return 0;
}

static int do_uaf(void) {
    int *p = malloc(64);
    if (!p) return 1;
    free(p);
    p[0] = 42;  // BUG on purpose: touches freed bytes
    printf("touched freed bytes: %d\n", p[0]);
    return 0;
}

int main(int argc, char **argv) {
    const char *which = argc > 1 ? argv[1] : "leak";
    if (strcmp(which, "leak") == 0) return do_leak();
    if (strcmp(which, "doublefree") == 0) return do_doublefree();
    if (strcmp(which, "uaf") == 0) return do_uaf();
    fprintf(stderr, "usage: %s [leak|doublefree|uaf]\n", argv[0]);
    return 2;
}

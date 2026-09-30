// args.c -- echo invocation, exit argc-1. Lesson docs/en.md.
#include <stdio.h>

int main(int argc, char **argv) {
    printf("argc=%d\n", argc);
    for (int i = 0; i < argc; i++)
        printf("argv[%d]=%s\n", i, argv[i]);
    return argc - 1;
}

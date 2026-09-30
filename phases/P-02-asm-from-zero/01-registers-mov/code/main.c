// main.c -- caller of ASM add2. Lesson docs/en.md.
#include <stdio.h>

int add2(int a, int b);

int main(void) {
    int r = add2(40, 2);
    printf("add2(40,2)=%d\n", r);
    return r != 42;
}

// model_add.c -- dataflow mirror of the template (no ASM). Lesson docs/en.md.
#include <stdio.h>

static int model_add(int x, int y) { int out = x; out = out + y; return out; }

int main(void) {
    printf("model_add(40,2)=%d\n", model_add(40, 2));
    return model_add(40, 2) != 42;
}

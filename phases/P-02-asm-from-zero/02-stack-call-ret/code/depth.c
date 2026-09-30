// depth.c -- rsp at three nesting levels. Lesson docs/en.md.
#include <stdio.h>

unsigned long get_rsp(void);

static void level2(void) {
    char pad[64];
    (void)pad;
    printf("level2 rsp=0x%lx\n", get_rsp());
}

static void level1(void) {
    char pad[64];
    (void)pad;
    printf("level1 rsp=0x%lx\n", get_rsp());
    level2();
}

int main(void) {
    printf("main   rsp=0x%lx\n", get_rsp());
    level1();
    return 0;
}

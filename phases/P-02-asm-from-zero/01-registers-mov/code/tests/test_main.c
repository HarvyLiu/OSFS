// test_main.c -- 3 checks on ASM add2 (linked with add.s).
#include <assert.h>
#include <stdio.h>

int add2(int a, int b);

int main(void) {
    assert(add2(40, 2) == 42);
    assert(add2(0, 0) == 0);
    assert(add2(-5, 5) == 0);
    assert(add2(1000000, 1000000) == 2000000);
    printf("all regs-mov checks pass\n");
    return 0;
}

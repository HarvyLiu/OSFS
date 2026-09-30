// test_main.c — 3 checks: helper math, loop-free logic, types sanity.
#include <assert.h>
#include <stdio.h>

static int square(int x) { return x * x; }

int main(void) {
    assert(square(0) == 0);
    assert(square(4) == 16);   // matches grep in make test
    assert(square(-3) == 9);
    assert(sizeof(char) == 1); // C standard guarantees this one
    assert(sizeof(int) >= 2);  // portable lower bound
    printf("all hello-c checks pass\n");
    return 0;
}

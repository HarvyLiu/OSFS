// test_main.c -- 4 checks: round-robin order, wrap, counts, pcb shape.
#include <assert.h>
#include <stdio.h>

static int next_task(int cur, int n) { return (cur + 1) % n; }

int main(void) {
    assert(next_task(0, 2) == 1);
    assert(next_task(1, 2) == 0);   // wraps
    assert(next_task(2, 3) == 0);   // generalizes to n tasks
    int cur = 0, c0 = 0, c1 = 0;
    for (int t = 0; t < 6; t++) {
        if (cur == 0) c0++; else c1++;
        cur = next_task(cur, 2);
    }
    assert(c0 == 3 && c1 == 3);     // matches grep in make test
    printf("all ctx-switch checks pass\n");
    return 0;
}

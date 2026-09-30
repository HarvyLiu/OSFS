// test_main.c -- 6 checks on the REAL engine (links mlfq_engine.c).
#include <assert.h>
#include <stdio.h>
#include "../mlfq.h"

int main(void) {
    mjob_t a[3] = {{1,0,3,0,0,0,0,0},{2,0,10,0,0,0,0,0},{3,0,3,0,0,0,0,0}};
    int order[3], no = 0;
    mlfq_run(a, 3, 0, order, &no);
    assert(a[0].comp == 7 && a[1].comp == 16 && a[2].comp == 12);
    assert(order[0] == 1 && order[1] == 3 && order[2] == 2);
    mjob_t b[3] = {{1,0,3,0,0,0,0,0},{2,0,10,0,0,0,0,0},{3,0,3,0,0,0,0,0}};
    mlfq_run(b, 3, 8, order, &no);
    assert(b[0].comp == 7 && b[1].comp == 16 && b[2].comp == 11);
    assert(order[0] == 1 && order[1] == 3 && order[2] == 2);
    // boost never delays the short jobs here; hog pays the same
    assert(b[2].comp <= a[2].comp);
    // single job finishes in exactly its burst
    mjob_t s[1] = {{9,0,5,0,0,0,0,0}};
    mlfq_run(s, 1, 0, order, &no);
    assert(s[0].comp == 5 && no == 1 && order[0] == 9);
    printf("all mlfq checks pass\n");
    return 0;
}

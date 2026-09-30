// test_main.c -- 6 checks on the real policies (links ffs.c).
#include <assert.h>
#include <stdio.h>
#include "../ffs.h"

static void fragment(ffs_t *f) {
    for (int b = 0; b <= 30; b += 2) ffs_mark(f, b);
    ffs_mark(f, 40);
}

int main(void) {
    ffs_t a, b;
    int fa[8], cl[8];
    ffs_init(&a);
    fragment(&a);
    assert(ffs_first_fit(&a, 8, fa) == 8);
    assert(fa[0] == 1 && fa[7] == 15);           // odds 1..15
    assert(ffs_span(fa, 8) == 14 && ffs_gaps(fa, 8) == 14);
    ffs_init(&b);
    fragment(&b);
    assert(ffs_clustered(&b, 40, 8, cl) == 8);
    assert(cl[0] == 41 && cl[7] == 48);          // contiguous run
    assert(ffs_span(cl, 8) == 7 && ffs_gaps(cl, 8) == 7);
    ffs_t c;
    ffs_init(&c);
    for (int i = 0; i < 64; i++) ffs_mark(&c, i);
    assert(ffs_first_fit(&c, 1, fa) == -1);      // full: honest -1
    assert(ffs_span(fa, 0) == 0 && ffs_gaps(fa, 1) == 0);  // edge math
    printf("all ffs-locality checks pass\n");
    return 0;
}

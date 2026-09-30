// main.c -- same holes, two policies. Lesson docs/en.md.
#include <stdio.h>
#include "ffs.h"

static void fragment(ffs_t *f) {
    for (int b = 0; b <= 30; b += 2) ffs_mark(f, b);  // evens 0..30: old holes
    ffs_mark(f, 40);                                   // the new file's inode block
}

int main(void) {
    ffs_t a, b;
    int fa[8], cl[8];
    ffs_init(&a);
    fragment(&a);
    ffs_first_fit(&a, 8, fa);
    ffs_init(&b);
    fragment(&b);
    ffs_clustered(&b, 40, 8, cl);
    printf("firstfit: span=%d gaps=%d\n", ffs_span(fa, 8), ffs_gaps(fa, 8));
    printf("clustered: span=%d gaps=%d\n", ffs_span(cl, 8), ffs_gaps(cl, 8));
    return 0;
}

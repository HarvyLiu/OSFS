// ffs.c -- placement policies over one bitmap. Lesson docs/en.md.
#include "ffs.h"

void ffs_init(ffs_t *f) {
    for (int i = 0; i < F_NBLOCKS; i++) f->used[i] = 0;
}

void ffs_mark(ffs_t *f, int bno) {
    if (bno >= 0 && bno < F_NBLOCKS) f->used[bno] = 1;
}

int ffs_first_fit(ffs_t *f, int n, int *out) {
    int got = 0;
    for (int i = 0; i < F_NBLOCKS && got < n; i++)
        if (!f->used[i]) {
            f->used[i] = 1;
            out[got++] = i;
        }
    return got == n ? n : -1;
}

int ffs_clustered(ffs_t *f, int near, int n, int *out) {
    int got = 0;
    for (int d = 0; d < F_NBLOCKS && got < n; d++) {
        int b = near + d;
        if (b >= 0 && b < F_NBLOCKS && !f->used[b]) {
            f->used[b] = 1;
            out[got++] = b;
        }
    }
    return got == n ? n : -1;
}

int ffs_span(const int *blocks, int n) {
    if (n <= 0) return 0;
    int lo = blocks[0], hi = blocks[0];
    for (int i = 1; i < n; i++) {
        if (blocks[i] < lo) lo = blocks[i];
        if (blocks[i] > hi) hi = blocks[i];
    }
    return hi - lo;
}

int ffs_gaps(const int *blocks, int n) {
    int s = 0;
    for (int i = 1; i < n; i++) {
        int d = blocks[i] - blocks[i - 1];
        s += d < 0 ? -d : d;
    }
    return s;
}

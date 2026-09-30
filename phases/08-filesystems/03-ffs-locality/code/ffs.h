// ffs.h -- first-fit vs clustered allocation contract. Lesson docs/en.md.
#ifndef OSFS_FFS_H
#define OSFS_FFS_H

#define F_NBLOCKS 64

typedef struct {
    int used[F_NBLOCKS];  // 1 = occupied
} ffs_t;

void ffs_init(ffs_t *f);
void ffs_mark(ffs_t *f, int bno);  // pre-occupy (old files' remains)
// out[] gets n blocks; returns n or -1. clustered scans up from near.
int ffs_first_fit(ffs_t *f, int n, int *out);
int ffs_clustered(ffs_t *f, int near, int n, int *out);
int ffs_span(const int *blocks, int n);
int ffs_gaps(const int *blocks, int n);

#endif

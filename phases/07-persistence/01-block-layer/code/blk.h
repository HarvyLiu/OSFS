// blk.h -- block device shape. Lesson docs/en.md.
#ifndef OSFS_BLK_H
#define OSFS_BLK_H

#include <stddef.h>

#define BLK_NBLOCKS 64
#define BLK_BSIZE 64

typedef struct {
    unsigned char cache[BLK_NBLOCKS][BLK_BSIZE];
    int dirty[BLK_NBLOCKS];
    const char *path;
    long reads, writes, flushes, drops;
} blkdev_t;

int blk_init(blkdev_t *d, const char *path);
int blk_read(blkdev_t *d, int bno, unsigned char *out);
int blk_write(blkdev_t *d, int bno, const unsigned char *in);
int blk_flush(blkdev_t *d);
int blk_crash(blkdev_t *d);

#endif

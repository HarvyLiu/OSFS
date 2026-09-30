// blk.c -- writeback cache over a flat file. Lesson docs/en.md.
#include <stdio.h>
#include <string.h>
#include "blk.h"

static long fsize(FILE *f) {
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    return n;
}

int blk_init(blkdev_t *d, const char *path) {
    memset(d, 0, sizeof *d);
    d->path = path;
    FILE *f = fopen(path, "r+b");
    if (!f) {
        f = fopen(path, "w+b");
        if (!f) return -1;
    }
    if (fsize(f) < (long)(BLK_NBLOCKS * BLK_BSIZE)) {
        unsigned char z[BLK_BSIZE] = {0};
        for (int i = 0; i < BLK_NBLOCKS; i++) fwrite(z, 1, BLK_BSIZE, f);
    }
    rewind(f);
    for (int i = 0; i < BLK_NBLOCKS; i++)
        if (fread(d->cache[i], 1, BLK_BSIZE, f) != BLK_BSIZE) { fclose(f); return -1; }
    fclose(f);
    return 0;
}

int blk_read(blkdev_t *d, int bno, unsigned char *out) {
    if (bno < 0 || bno >= BLK_NBLOCKS) return -1;
    memcpy(out, d->cache[bno], BLK_BSIZE);
    d->reads++;
    return 0;
}

int blk_write(blkdev_t *d, int bno, const unsigned char *in) {
    if (bno < 0 || bno >= BLK_NBLOCKS) return -1;
    memcpy(d->cache[bno], in, BLK_BSIZE);
    d->dirty[bno] = 1;
    d->writes++;
    return 0;
}

int blk_flush(blkdev_t *d) {
    FILE *f = fopen(d->path, "r+b");
    if (!f) return -1;
    int n = 0;
    for (int i = 0; i < BLK_NBLOCKS; i++)
        if (d->dirty[i]) {
            fseek(f, (long)i * BLK_BSIZE, SEEK_SET);
            fwrite(d->cache[i], 1, BLK_BSIZE, f);
            d->dirty[i] = 0;
            n++;
        }
    fclose(f);
    d->flushes++;
    return n;
}

int blk_crash(blkdev_t *d) {
    FILE *f = fopen(d->path, "rb");
    if (!f) return -1;
    int n = 0;
    for (int i = 0; i < BLK_NBLOCKS; i++)
        if (d->dirty[i]) {
            fseek(f, (long)i * BLK_BSIZE, SEEK_SET);
            if (fread(d->cache[i], 1, BLK_BSIZE, f) != BLK_BSIZE) { fclose(f); return -1; }
            d->dirty[i] = 0;
            n++;
        }
    fclose(f);
    d->drops++;
    return n;
}

// ramdisk.c -- malloc platters. Lesson docs/en.md.
#include <stdlib.h>
#include <string.h>
#include "ramdisk.h"

int rd_init(ramdisk_t *d) {
    memset(d, 0, sizeof *d);
    d->store = calloc(RD_NBLOCKS, RD_BSIZE);
    if (!d->store) return -1;
    d->live = 1;
    return 0;
}

int rd_read(ramdisk_t *d, int bno, unsigned char *out) {
    if (!d->live || bno < 0 || bno >= RD_NBLOCKS) return -1;
    memcpy(out, d->store + (size_t)bno * RD_BSIZE, RD_BSIZE);
    d->reads++;
    return 0;
}

int rd_write(ramdisk_t *d, int bno, const unsigned char *in) {
    if (!d->live || bno < 0 || bno >= RD_NBLOCKS) return -1;
    memcpy(d->store + (size_t)bno * RD_BSIZE, in, RD_BSIZE);
    d->dirty[bno] = 1;
    d->writes++;
    return 0;
}

int rd_flush(ramdisk_t *d) {
    if (!d->live) return -1;
    for (int i = 0; i < RD_NBLOCKS; i++) d->dirty[i] = 0;
    d->flushes++;
    return 0;
}

int rd_crash(ramdisk_t *d) {
    if (!d->live) return -1;
    memset(d->store, 0, sizeof(unsigned char) * RD_NBLOCKS * RD_BSIZE);
    memset(d->dirty, 0, sizeof d->dirty);
    d->drops++;
    return 0;
}

void rd_free(ramdisk_t *d) {
    free(d->store);
    d->store = 0;
    d->live = 0;
}

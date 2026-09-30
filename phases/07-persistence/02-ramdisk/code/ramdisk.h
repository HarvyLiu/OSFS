// ramdisk.h -- same verbs, RAM behind. Lesson docs/en.md.
#ifndef OSFS_RAMDISK_H
#define OSFS_RAMDISK_H

#include <stddef.h>

#define RD_NBLOCKS 64
#define RD_BSIZE 64

typedef struct {
    unsigned char *store;
    int dirty[RD_NBLOCKS];
    long reads, writes, flushes, drops;
    int live;
} ramdisk_t;

int rd_init(ramdisk_t *d);
int rd_read(ramdisk_t *d, int bno, unsigned char *out);
int rd_write(ramdisk_t *d, int bno, const unsigned char *in);
int rd_flush(ramdisk_t *d);
int rd_crash(ramdisk_t *d);
void rd_free(ramdisk_t *d);

#endif

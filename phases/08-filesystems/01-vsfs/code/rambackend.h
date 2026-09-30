// rambackend.h -- RAM blocks behind vsfs ops. Lesson docs/en.md.
#ifndef OSFS_RAMBACKEND_H
#define OSFS_RAMBACKEND_H

#include <string.h>
#include "vsfs.h"

typedef struct { unsigned char d[VSB_NBLOCKS][VSB_BSIZE]; } ramdisk_mem_t;

static int ram_r(void *ctx, int bno, unsigned char *out) {
    if (bno < 0 || bno >= VSB_NBLOCKS) return -1;
    memcpy(out, ((ramdisk_mem_t *)ctx)->d[bno], VSB_BSIZE);
    return 0;
}

static int ram_w(void *ctx, int bno, const unsigned char *in) {
    if (bno < 0 || bno >= VSB_NBLOCKS) return -1;
    memcpy(((ramdisk_mem_t *)ctx)->d[bno], in, VSB_BSIZE);
    return 0;
}

#endif

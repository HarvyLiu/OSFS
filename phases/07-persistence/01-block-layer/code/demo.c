// demo.c -- crash loses unflushed; flush survives crash. Lesson docs/en.md.
#include <stdio.h>
#include <string.h>
#include "blk.h"

static void fill(unsigned char *b, char c) { memset(b, c, BLK_BSIZE); }

int main(void) {
    blkdev_t d;
    remove("build/disk.img");
    if (blk_init(&d, "build/disk.img") != 0) return 1;
    unsigned char b[BLK_BSIZE], out[BLK_BSIZE];
    fill(b, 'A');
    blk_write(&d, 3, b);
    printf("dropped=%d ", blk_crash(&d));
    blk_read(&d, 3, out);
    printf("after-crash-1st-byte=%d (0=zeroed, 65=A)\n", out[0]);
    fill(b, 'B');
    blk_write(&d, 5, b);
    printf("flushed=%d ", blk_flush(&d));
    printf("dropped=%d ", blk_crash(&d));
    blk_read(&d, 5, out);
    printf("after-flush-1st-byte=%d\n", out[0]);
    printf("reads=%ld writes=%ld flushes=%ld drops=%ld\n",
           d.reads, d.writes, d.flushes, d.drops);
    return out[0] != 'B';
}

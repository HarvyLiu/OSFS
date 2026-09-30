// demo.c -- coherence yes, durability no. Lesson docs/en.md.
#include <stdio.h>
#include <string.h>
#include "ramdisk.h"

int main(void) {
    ramdisk_t d;
    if (rd_init(&d) != 0) return 1;
    unsigned char b[RD_BSIZE], out[RD_BSIZE];
    memset(b, 'R', sizeof b);
    rd_write(&d, 10, b);
    rd_read(&d, 10, out);
    printf("pre-crash=%d (82=R)\n", out[0]);
    rd_crash(&d);
    rd_read(&d, 10, out);
    printf("post-crash=%d (0=wiped) reads=%ld writes=%ld flushes=%ld drops=%ld\n",
           out[0], d.reads, d.writes, d.flushes, d.drops);
    rd_free(&d);
    return out[0] != 0;
}

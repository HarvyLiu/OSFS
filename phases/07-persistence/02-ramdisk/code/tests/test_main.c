// test_main.c -- 6 checks: roundtrip, wipe, post-free, OOB, flush-noop, edge block.
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../ramdisk.h"

int main(void) {
    ramdisk_t d;
    assert(rd_init(&d) == 0);
    unsigned char w[RD_BSIZE], r[RD_BSIZE];
    memset(w, 'Q', sizeof w);
    assert(rd_write(&d, 63, w) == 0);   // edge block counts
    assert(rd_read(&d, 63, r) == 0 && r[0] == 'Q');
    assert(rd_write(&d, -1, w) == -1 && rd_read(&d, 64, r) == -1);  // bounds
    assert(rd_flush(&d) == 0);          // theater, succeeds
    assert(rd_crash(&d) == 0);
    assert(rd_read(&d, 63, r) == 0 && r[0] == 0);  // amnesia
    rd_free(&d);
    assert(rd_read(&d, 0, r) == -1 && rd_write(&d, 0, w) == -1);  // dead device
    printf("all ramdisk checks pass\n");
    return 0;
}

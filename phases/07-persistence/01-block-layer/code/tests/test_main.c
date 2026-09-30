// test_main.c -- 6 checks on the real cache (links blk.c).
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../blk.h"

int main(void) {
    remove("build/test-disk.img");
    blkdev_t d;
    assert(blk_init(&d, "build/test-disk.img") == 0);
    unsigned char w[BLK_BSIZE], r[BLK_BSIZE];
    memset(w, 'Q', sizeof w);
    assert(blk_write(&d, 7, w) == 0);
    assert(blk_read(&d, 7, r) == 0 && r[0] == 'Q');  // cached read sees dirt
    assert(blk_write(&d, -1, w) == -1 && blk_read(&d, 64, r) == -1);  // bounds
    assert(blk_flush(&d) == 1);   // exactly one dirty
    blkdev_t d2;
    assert(blk_init(&d2, "build/test-disk.img") == 0);  // reopen: persistence
    assert(blk_read(&d2, 7, r) == 0 && r[0] == 'Q');
    memset(w, 'Z', sizeof w);
    assert(blk_write(&d2, 9, w) == 0);
    assert(blk_crash(&d2) == 1);  // one dirty dropped
    assert(blk_read(&d2, 9, r) == 0 && r[0] == 0);  // loss: back to zeroed
    remove("build/test-disk.img");
    printf("all block-layer checks pass\n");
    return 0;
}

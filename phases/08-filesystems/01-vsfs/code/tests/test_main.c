// test_main.c -- 7 checks on the real FS (links vsfs.c, RAM backend).
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../vsfs.h"
#include "../rambackend.h"

int main(void) {
    ramdisk_mem_t mem;
    memset(&mem, 0, sizeof mem);
    vsfs_ops_t ops = {ram_r, ram_w, &mem};
    assert(vsfs_mkfs(&ops) == 0);
    int h = vsfs_create(&ops, "hello");
    assert(h == 1);  // root is 0, first file is 1
    assert(vsfs_create(&ops, "hello") == -1);  // dupes refused
    const char *msg = "hello vsfs!";
    assert(vsfs_write(&ops, h, (const unsigned char *)msg, strlen(msg)) == 0);
    unsigned char out[64];
    int n = vsfs_read(&ops, h, out, sizeof out - 1);
    assert(n == 11);
    out[n] = 0;
    assert(strcmp((char *)out, msg) == 0);
    assert(vsfs_read(&ops, 5, out, sizeof out) == -1);  // never created
    unsigned char big[VSB_MAXFILE + 44];
    memset(big, 'x', sizeof big);
    assert(vsfs_write(&ops, h, big, sizeof big) == -1);  // over ceiling
    assert(vsfs_used_blocks(&ops) == 8);  // super+bitmap+4 inode+root data+1 file
    printf("all vsfs checks pass\n");
    return 0;
}

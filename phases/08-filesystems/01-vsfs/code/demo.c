// demo.c -- filesystem first day: format, name, fill, verify, census.
#include <stdio.h>
#include <string.h>
#include "vsfs.h"
#include "rambackend.h"

int main(void) {
    ramdisk_mem_t mem;
    memset(&mem, 0, sizeof mem);
    vsfs_ops_t ops = {ram_r, ram_w, &mem};
    if (vsfs_mkfs(&ops) != 0) { printf("mkfs failed\n"); return 1; }
    printf("mkfs: ok\n");
    int h = vsfs_create(&ops, "hello");
    if (h < 0) { printf("create failed\n"); return 1; }
    printf("create hello -> inum %d\n", h);
    const char *msg = "hello vsfs!";
    if (vsfs_write(&ops, h, (const unsigned char *)msg, strlen(msg)) < 0) {
        printf("write failed\n");
        return 1;
    }
    unsigned char out[64];
    int n = vsfs_read(&ops, h, out, sizeof out - 1);
    if (n < 0) { printf("read failed\n"); return 1; }
    out[n] = 0;
    printf("read back: %s (size %d)\n", out, n);
    vsfs_list(&ops);
    printf("blocks used: %d/%d\n", vsfs_used_blocks(&ops), VSB_NBLOCKS);
    return 0;
}

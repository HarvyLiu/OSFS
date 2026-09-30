// main.c -- three mappings, four walks. Lesson docs/en.md.
#include <stdio.h>
#include "pt.h"

int main(void) {
    pdir_t *root = pt_new();
    if (!root) return 1;
    pt_map(root, 0x00401000, 0x00007000, PTE_RW | PTE_US);
    pt_map(root, 0x00402000, 0x00008000, PTE_US);
    pt_map(root, 0xC0000000, 0x00100000, PTE_RW);
    uint32_t pa = 0;
    // NOTE: call-then-print (C printf arg order is unspecified; calling inside
    // prints stale pa -- the same bug class 05/02 documents).
    int r1 = pt_translate(root, 0x00401ABC, 0, &pa);
    printf("read 0x00401ABC -> %s 0x%x\n", r1 == 0 ? "ok" : "FAULT", pa);
    int r2 = pt_translate(root, 0x00402ABC, 1, &pa);
    printf("write 0x00402ABC -> %s (RO page)\n", r2 == 0 ? "ok" : "FAULT");
    int r3 = pt_translate(root, 0x00800000, 0, &pa);
    printf("read 0x00800000 -> %s (never mapped)\n", r3 == 0 ? "ok" : "FAULT");
    int r4 = pt_translate(root, 0xC0000ABC, 0, &pa);
    printf("read 0xC0000ABC -> %s 0x%x (high half)\n", r4 == 0 ? "ok" : "FAULT", pa);
    pt_free(root);
    return !(r1 == 0 && r2 == -4 && r3 != 0 && r4 == 0);
}

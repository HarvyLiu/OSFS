// test_main.c -- 7 checks on the real walker (links pt.c).
#include <assert.h>
#include <stdio.h>
#include "../pt.h"

int main(void) {
    pdir_t *r = pt_new();
    assert(r != 0);
    assert(pt_map(r, 0x00401000, 0x00007000, PTE_RW | PTE_US) == 0);
    assert(pt_map(r, 0x00402000, 0x00008000, PTE_US) == 0);
    uint32_t pa = 0;
    assert(pt_translate(r, 0x00401ABC, 0, &pa) == 0 && pa == 0x7ABC);
    assert(pt_translate(r, 0x00402ABC, 1, &pa) == -4);  // RO write
    assert(pt_translate(r, 0x00402ABC, 0, &pa) == 0 && pa == 0x8ABC);  // RO read ok
    assert(pt_translate(r, 0x00800000, 0, &pa) == -2);  // dir miss (pd=2 empty)
    assert(pt_unmap(r, 0x00401000) == 0);
    assert(pt_translate(r, 0x00401000, 0, &pa) == -3);  // cleared -> page miss
    assert(pt_map(r, 0x00401000, 0x00009000, PTE_RW | PTE_US) == 0);  // remap
    assert(pt_translate(r, 0x00401FFF, 0, &pa) == 0 && pa == 0x9FFF);  // max offset
    assert(pt_map(r, 0xC0000000, 0x00100000, PTE_RW) == 0);
    assert(pt_translate(r, 0xC0000ABC, 0, &pa) == 0 && pa == 0x100ABC);  // high half
    pt_free(r);
    printf("all page-tables checks pass\n");
    return 0;
}

// test_main.c -- 6 checks: seg ok/limit/badseg, page ok/present/range, offset passthrough.
#include <assert.h>
#include <stdio.h>

#define PGSIZE 4096
#define NSEG 3
#define NPT 8

static unsigned seg_base[NSEG] = {0x10000, 0x20000, 0x30000};
static unsigned seg_lim[NSEG] = {0x4000, 0x2000, 0x1000};
static int ptab[NPT] = {5, -1, 7, 3, -1, -1, 9, -1};

static long seg_translate(int seg, unsigned off, unsigned *phys) {
    if (seg < 0 || seg >= NSEG) return -1;
    if (off >= seg_lim[seg]) return -2;
    *phys = seg_base[seg] + off;
    return 0;
}

static long page_translate(unsigned va, unsigned *phys) {
    unsigned vpn = va >> 12;
    unsigned off = va & 0xFFF;
    if (vpn >= NPT) return -1;
    if (ptab[vpn] < 0) return -2;
    *phys = (unsigned)ptab[vpn] * PGSIZE + off;
    return 0;
}

int main(void) {
    unsigned p = 0;
    assert(seg_translate(1, 0x100, &p) == 0 && p == 0x20100);
    assert(seg_translate(1, 0x2000, &p) == -2);  // limit exclusive
    assert(seg_translate(9, 0, &p) == -1);       // bad segment
    assert(page_translate(0x2ABC, &p) == 0 && p == 0x7ABC);
    assert(page_translate(0x1ABC, &p) == -2);    // vpn1 !present
    assert(page_translate(0x8000, &p) == -1);    // vpn8: outside table
    assert(page_translate(0x2FFF, &p) == 0 && (p & 0xFFF) == 0xFFF);  // offset passthrough
    printf("all seg-paging checks pass\n");
    return 0;
}

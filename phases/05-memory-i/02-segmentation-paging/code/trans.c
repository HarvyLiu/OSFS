// trans.c -- seg + flat-page translators. Lesson docs/en.md.
#include <stdio.h>

#define PGSIZE 4096
#define NSEG 3
#define NPT 8

static unsigned seg_base[NSEG] = {0x10000, 0x20000, 0x30000};
static unsigned seg_lim[NSEG] = {0x4000, 0x2000, 0x1000};

// flat page table: frame number, -1 = not present
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
    int rc;
    // NOTE: call-then-print (never inside printf args: C arg order is unspecified,
    // and p would print stale). This exact bug was caught by verification.
    rc = seg_translate(1, 0x100, &p);
    printf("seg(1,0x100)=%s 0x%x\n", rc == 0 ? "ok" : "FAULT", p);
    rc = seg_translate(1, 0x2000, &p);
    printf("seg(1,0x2000)=%s (limit 0x2000)\n", rc == 0 ? "ok" : "FAULT");
    rc = page_translate(0x1ABC, &p);
    printf("page(0x1ABC)=%s (vpn1 !present)\n", rc == 0 ? "ok" : "FAULT");
    rc = page_translate(0x2ABC, &p);
    printf("page(0x2ABC)=%s 0x%x (frame7+0xABC)\n", rc == 0 ? "ok" : "FAULT", p);
    rc = page_translate(0x6123, &p);
    printf("page(0x6123)=%s 0x%x (frame9+0x123)\n", rc == 0 ? "ok" : "FAULT", p);
    return 0;
}

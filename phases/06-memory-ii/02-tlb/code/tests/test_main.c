// test_main.c -- 5 checks: counters, EAT, FIFO eviction, hot trace, fault path.
#include <assert.h>
#include <stdio.h>

#define NPT 16
#define NTLB 4

static int ptab[NPT];
typedef struct { int vpn; int frame; int valid; } tlb_e;
static tlb_e tlb[NTLB];
static int tlb_next;
static long hits, misses;

static void tlb_reset(void) {
    for (int i = 0; i < NTLB; i++) tlb[i].valid = 0;
    tlb_next = 0;
    hits = misses = 0;
}

static int access(int vpn) {
    for (int i = 0; i < NTLB; i++)
        if (tlb[i].valid && tlb[i].vpn == vpn) { hits++; return 0; }
    if (vpn < 0 || vpn >= NPT || ptab[vpn] < 0) return -1;
    tlb[tlb_next].vpn = vpn;
    tlb[tlb_next].frame = ptab[vpn];
    tlb[tlb_next].valid = 1;
    tlb_next = (tlb_next + 1) % NTLB;
    misses++;
    return 0;
}

int main(void) {
    for (int i = 0; i < NPT; i++) ptab[i] = i + 10;
    int trace[] = {0, 1, 2, 3, 0, 1, 4, 0};
    for (int i = 0; i < 8; i++) assert(access(trace[i]) == 0);
    assert(hits == 2 && misses == 6);
    assert((hits * 101 + misses * 201) / 8 == 176);  // EAT
    tlb_reset();
    ptab[5] = -1;
    assert(access(5) == -1);  // fault path inserts nothing
    assert(hits == 0 && misses == 0);
    ptab[5] = 15;
    tlb_reset();
    int hot[] = {0, 0, 0, 0, 1, 1, 1, 1};
    for (int i = 0; i < 8; i++) assert(access(hot[i]) == 0);
    assert(hits == 6 && misses == 2);  // locality rewarded
    printf("all tlb checks pass\n");
    return 0;
}

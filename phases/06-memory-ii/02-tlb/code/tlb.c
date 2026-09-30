// tlb.c -- 4-entry FIFO TLB over a flat table. Lesson docs/en.md.
#include <stdio.h>

#define NPT 16
#define NTLB 4
#define T_HIT 101
#define T_MISS 201

static int ptab[NPT];

typedef struct { int vpn; int frame; int valid; } tlb_e;
static tlb_e tlb[NTLB];
static int tlb_next = 0;
static long hits = 0, misses = 0;

static int tlb_lookup(int vpn, int *frame) {
    for (int i = 0; i < NTLB; i++)
        if (tlb[i].valid && tlb[i].vpn == vpn) {
            *frame = tlb[i].frame;
            hits++;
            return 1;
        }
    return 0;
}

static void tlb_insert(int vpn, int frame) {
    tlb[tlb_next].vpn = vpn;
    tlb[tlb_next].frame = frame;
    tlb[tlb_next].valid = 1;
    tlb_next = (tlb_next + 1) % NTLB;
    misses++;
}

static int access(int vpn) {
    int frame;
    if (tlb_lookup(vpn, &frame)) return 0;
    if (vpn < 0 || vpn >= NPT || ptab[vpn] < 0) return -1;
    tlb_insert(vpn, ptab[vpn]);
    return 0;
}

int main(void) {
    for (int i = 0; i < NPT; i++) ptab[i] = i + 10;
    int trace[] = {0, 1, 2, 3, 0, 1, 4, 0};
    for (int i = 0; i < 8; i++)
        if (access(trace[i]) != 0) { printf("fault?!\n"); return 1; }
    long total = hits + misses;
    long eat = (hits * T_HIT + misses * T_MISS) / total;
    printf("hits=%ld misses=%ld eat=%ld\n", hits, misses, eat);
    return hits != 2 || misses != 6 || eat != 176;
}

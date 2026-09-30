// demand.c -- FIFO eviction to swap, dirty writebacks. Lesson docs/en.md.
#include "demand.h"

void demand_init(demand_t *d) {
    for (int i = 0; i < D_NFRAMES; i++) {
        d->frames[i] = -1;
        d->dirty[i] = 0;
    }
    d->fifo = 0;
    d->faults = d->evictions = d->writebacks = 0;
}

int demand_access(demand_t *d, int vpn, int is_write) {
    if (vpn < 0 || vpn >= D_NPAGES) return -1;  // unmapped address, not just swapped
    for (int i = 0; i < D_NFRAMES; i++)
        if (d->frames[i] == vpn) {
            if (is_write) d->dirty[i] = 1;
            return 0;  // hit: resident
        }
    d->faults++;  // major fault: load from disk (always succeeds here)
    int v = d->fifo;
    if (d->frames[v] != -1) {
        d->evictions++;
        if (d->dirty[v]) d->writebacks++;  // dirty out first!
    }
    d->frames[v] = vpn;
    d->dirty[v] = is_write ? 1 : 0;
    d->fifo = (d->fifo + 1) % D_NFRAMES;
    return 1;
}

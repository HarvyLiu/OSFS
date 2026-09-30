// demand.h -- demand paging + swap engine contract. Lesson docs/en.md.
#ifndef OSFS_DEMAND_H
#define OSFS_DEMAND_H

#define D_NPAGES 6
#define D_NFRAMES 3

typedef struct {
    int frames[D_NFRAMES];  // vpn per frame, -1 = empty
    int dirty[D_NFRAMES];   // 1 = needs writeback on evict
    int fifo;               // next victim hand
    long faults;
    long evictions;
    long writebacks;
} demand_t;

void demand_init(demand_t *d);
// Returns 1 on fault (major), 0 on hit.
int demand_access(demand_t *d, int vpn, int is_write);

#endif

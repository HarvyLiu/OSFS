// main.c -- fixed trace through 3 frames. Lesson docs/en.md.
#include <stdio.h>
#include "demand.h"

int main(void) {
    demand_t d;
    demand_init(&d);
    // (vpn, write)
    int trace[][2] = {
        {0,0},{1,0},{2,0},          // fill: 3 faults
        {0,0},{1,0},                // hot: hits
        {3,0},                      // evict 0 (clean)
        {0,1},{1,0},                // 0 back (evict 1), dirty 0
        {4,0},                      // evict 2
        {0,0},{1,0},{2,0},          // 0 hit; 1 back (evict 3); 2 back (evict dirty 0 -> writeback)
        {5,0},                      // evict 4
    };
    int n = sizeof(trace) / sizeof(trace[0]);
    int majors = 0;
    for (int i = 0; i < n; i++)
        majors += demand_access(&d, trace[i][0], trace[i][1]);
    printf("faults=%ld evictions=%ld writebacks=%ld resident=",
           d.faults, d.evictions, d.writebacks);
    for (int i = 0; i < D_NFRAMES; i++) printf(" %d", d.frames[i]);
    printf(" majors=%d/%d\n", majors, n);
    return 0;
}

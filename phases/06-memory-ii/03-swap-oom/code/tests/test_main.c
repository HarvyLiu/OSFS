// test_main.c -- 6 checks on the real engine (links demand.c).
#include <assert.h>
#include <stdio.h>
#include "../demand.h"

int main(void) {
    demand_t d;
    demand_init(&d);
    int trace[][2] = {
        {0,0},{1,0},{2,0},{0,0},{1,0},{3,0},{0,1},{1,0},
        {4,0},{0,0},{1,0},{2,0},{5,0},
    };
    for (int i = 0; i < 13; i++) assert(demand_access(&d, trace[i][0], trace[i][1]) >= 0);
    assert(d.faults == 9 && d.evictions == 6 && d.writebacks == 1);
    assert(d.frames[0] == 4 && d.frames[1] == 2 && d.frames[2] == 5);  // resident set
    assert(demand_access(&d, 9, 0) == -1);   // outside 0..5: unmapped address
    assert(demand_access(&d, -1, 0) == -1);
    demand_t e;
    demand_init(&e);
    assert(demand_access(&e, 0, 0) == 1);    // cold: fault, no eviction
    assert(e.faults == 1 && e.evictions == 0 && e.writebacks == 0);
    printf("all swap-oom checks pass\n");
    return 0;
}

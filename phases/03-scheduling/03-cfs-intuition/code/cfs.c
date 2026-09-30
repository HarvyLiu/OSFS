// cfs.c -- min-vruntime scheduler sketch. Lesson docs/en.md.
// Weights from Linux sched_prio_to_weight: nice -5 -> 3121, 0 -> 1024, 5 -> 335.
#include <stdio.h>

typedef struct { int id; int nice; int weight; double vr; int ticks; } ctask_t;

static int pick_min(ctask_t *ts, int n) {
    int m = 0;
    for (int i = 1; i < n; i++)
        if (ts[i].vr < ts[m].vr || (ts[i].vr == ts[m].vr && ts[i].id < ts[m].id))
            m = i;
    return m;
}

int main(void) {
    ctask_t ts[3] = {{1, 0, 1024, 0.0, 0}, {2, 5, 335, 0.0, 0}, {3, -5, 3121, 0.0, 0}};
    const int T = 60;
    for (int t = 0; t < T; t++) {
        int m = pick_min(ts, 3);
        ts[m].ticks++;
        ts[m].vr += 1024.0 / ts[m].weight;
    }
    for (int i = 0; i < 3; i++)
        printf("task %d nice=%d weight=%d ticks=%d vr=%.2f\n",
               ts[i].id, ts[i].nice, ts[i].weight, ts[i].ticks, ts[i].vr);
    return 0;
}

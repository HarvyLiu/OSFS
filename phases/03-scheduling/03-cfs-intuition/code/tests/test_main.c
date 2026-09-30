// test_main.c -- 6 checks: shares, conservation, order, bounded lag. No libm.
#include <assert.h>
#include <stdio.h>

typedef struct { int id; int nice; int weight; double vr; int ticks; } ctask_t;

static int pick_min(ctask_t *ts, int n) {
    int m = 0;
    for (int i = 1; i < n; i++)
        if (ts[i].vr < ts[m].vr || (ts[i].vr == ts[m].vr && ts[i].id < ts[m].id))
            m = i;
    return m;
}

static int close(double a, double b, double eps) {
    double d = a > b ? a - b : b - a;
    return d < eps;
}

int main(void) {
    ctask_t ts[3] = {{1, 0, 1024, 0.0, 0}, {2, 5, 335, 0.0, 0}, {3, -5, 3121, 0.0, 0}};
    for (int t = 0; t < 60; t++) {
        int m = pick_min(ts, 3);
        ts[m].ticks++;
        ts[m].vr += 1024.0 / ts[m].weight;
    }
    assert(ts[0].ticks == 14 && ts[1].ticks == 5 && ts[2].ticks == 41);
    assert(ts[0].ticks + ts[1].ticks + ts[2].ticks == 60);  // conserved
    assert(ts[2].ticks > ts[0].ticks && ts[0].ticks > ts[1].ticks);  // weight order
    assert(close(ts[0].vr, 14.0, 0.01) && close(ts[2].vr, 13.45, 0.05));
    double lo = ts[2].vr, hi = ts[1].vr;  // min/max known from run
    assert(hi - lo < 4.0);  // bounded lag: one big step (3.06) + slack
    // equal weights degrade to round-robin
    ctask_t eq[3] = {{1, 0, 1024, 0.0, 0}, {2, 0, 1024, 0.0, 0}, {3, 0, 1024, 0.0, 0}};
    for (int t = 0; t < 60; t++) {
        int m = pick_min(eq, 3);
        eq[m].ticks++;
        eq[m].vr += 1.0;
    }
    assert(eq[0].ticks == 20 && eq[1].ticks == 20 && eq[2].ticks == 20);
    printf("all cfs checks pass\n");
    return 0;
}

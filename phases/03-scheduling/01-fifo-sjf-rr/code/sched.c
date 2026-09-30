// sched.c -- FIFO/SJF/RR(q=2) on fixed workload. Lesson docs/en.md.
// P1(arr0,b8) P2(arr1,b4) P3(arr2,b2). Completions hand-traced in docs.
#include <stdio.h>

typedef struct { int id, arr, burst, comp; } job_t;

static void report(const char *name, job_t *js, int n) {
    double tsum = 0, wsum = 0;
    printf("%s order:", name);
    for (int i = 0; i < n; i++) printf(" P%d", js[i].id);
    printf(" comp:");
    for (int i = 0; i < n; i++) printf(" P%d=%d", js[i].id, js[i].comp);
    for (int i = 0; i < n; i++) {
        int turn = js[i].comp - js[i].arr;
        tsum += turn;
        wsum += turn - js[i].burst;
    }
    printf(" avg_turn=%.2f avg_wait=%.2f\n", tsum / n, wsum / n);
}

int main(void) {
    job_t fifo[3] = {{1,0,8,8},{2,1,4,12},{3,2,2,14}};
    job_t sjf[3]  = {{1,0,8,8},{3,2,2,10},{2,1,4,14}};
    job_t rr[3]   = {{1,0,8,14},{2,1,4,10},{3,2,2,6}};
    report("FIFO", fifo, 3);
    report("SJF", sjf, 3);
    report("RRq2", rr, 3);
    return 0;
}

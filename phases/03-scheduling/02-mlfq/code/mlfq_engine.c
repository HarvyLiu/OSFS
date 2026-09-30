// mlfq_engine.c -- 3-queue MLFQ: demote on exhaust, FIFO within, boost. Lesson docs/en.md.
#include "mlfq.h"

static const int QQUANTUM[3] = {2, 4, 1 << 30};

void mlfq_run(mjob_t *js, int n, int boost_every, int *order, int *norder) {
    int t = 0, ndone = 0, no = 0;
    for (int i = 0; i < n; i++) {
        js[i].rem = js[i].burst;
        js[i].q = 0;
        js[i].qu = 0;
        js[i].qs = 0;
        js[i].comp = 0;
    }
    while (ndone < n) {
        int pick = -1;
        for (int i = 0; i < n; i++) {
            if (js[i].arr > t || js[i].comp) continue;
            if (pick < 0 || js[i].q < js[pick].q ||
                (js[i].q == js[pick].q && (js[i].qs < js[pick].qs ||
                 (js[i].qs == js[pick].qs && js[i].id < js[pick].id))))
                pick = i;
        }
        mjob_t *j = &js[pick];
        j->rem--;
        j->qu++;
        t++;
        if (j->rem == 0) {
            j->comp = t;
            order[no++] = j->id;
            ndone++;
        } else if (j->qu >= QQUANTUM[j->q]) {
            if (j->q < 2) j->q++;
            j->qu = 0;
            j->qs = t;
        }
        if (boost_every > 0 && t % boost_every == 0) {
            for (int i = 0; i < n; i++)
                if (!js[i].comp) { js[i].q = 0; js[i].qu = 0; js[i].qs = t; }
        }
    }
    *norder = no;
}

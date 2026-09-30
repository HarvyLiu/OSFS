// mlfq.h -- engine contract. Lesson docs/en.md.
#ifndef OSFS_MLFQ_H
#define OSFS_MLFQ_H

typedef struct {
    int id;      // job identity
    int arr;     // arrival tick
    int burst;   // total CPU needed
    int rem;     // remaining (engine-owned)
    int q;       // current queue 0..2 (engine-owned)
    int qu;      // quantum used in current queue (engine-owned)
    int qs;      // tick entered current queue: FIFO within (engine-owned)
    int comp;    // completion tick (engine-owned)
} mjob_t;

// Run to completion. boost_every<=0 disables boost.
// order[] receives finish order (ids), norder its length.
void mlfq_run(mjob_t *js, int n, int boost_every, int *order, int *norder);

#endif

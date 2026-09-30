// pcb-template.h -- drop-in starting point for Phase 10 kernel.
// Swap snprintf for serial-safe copy + u64 rsp when freestanding.
#ifndef OSFS_PCB_H
#define OSFS_PCB_H

typedef enum { P_UNUSED = 0, P_RUNNABLE, P_RUNNING, P_ZOMBIE } pstate_t;

typedef struct {
    int pid;
    pstate_t state;
    unsigned long rsp;
    char name[16];
} pcb_t;

pcb_t *pcb_alloc(int pid, const char *name);
pcb_t *pcb_find(int pid);
void pcb_reap(pcb_t *p);

#endif

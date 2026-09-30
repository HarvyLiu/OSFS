// trapframe.h -- drop-in for Phase 10 kernel. Lesson 02/01.
// Grow NREG to full set, add files/cwd, keep save-all/restore-reverse order.
#ifndef OSFS_TRAPFRAME_H
#define OSFS_TRAPFRAME_H

#define TF_NREG 8

typedef struct {
    long regs[TF_NREG];
    long pc;
    long sp;
    long flags;
} trapframe_t;

typedef enum { P_UNUSED = 0, P_RUNNABLE, P_RUNNING, P_ZOMBIE } pstate_t;

typedef struct {
    int pid;
    int ppid;
    pstate_t state;
    trapframe_t tf;
    unsigned long pageroot;
    char name[16];
} pcb_t;

#endif

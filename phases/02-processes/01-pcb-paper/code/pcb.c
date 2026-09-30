// pcb.c -- save/clobber/restore round-trip. Lesson docs/en.md.
#include <stdio.h>
#include <string.h>

#define NREG 8

typedef struct {
    long regs[NREG];
    long pc;
    long sp;
    long flags;
} trapframe_t;

typedef enum { UNUSED = 0, RUNNABLE, RUNNING, ZOMBIE } state_t;

typedef struct {
    int pid;
    int ppid;
    state_t state;
    trapframe_t tf;
    unsigned long pageroot;
    char name[16];
} pcb_t;

static void trap_save(pcb_t *p, long pc, long sp) {
    for (int i = 0; i < NREG; i++) p->tf.regs[i] = 100 + i;
    p->tf.pc = pc;
    p->tf.sp = sp;
    p->tf.flags = 0x202;
}

static void clobber(pcb_t *p) {
    for (int i = 0; i < NREG; i++) p->tf.regs[i] = -1;
    p->tf.pc = 0;
}

static void trap_restore(pcb_t *p, const trapframe_t *saved) {
    p->tf = *saved;
}

int main(void) {
    pcb_t p;
    memset(&p, 0, sizeof p);
    p.pid = 7;
    p.ppid = 1;
    p.state = RUNNING;
    p.pageroot = 0x100000;
    snprintf(p.name, 16, "%s", "demo");
    trap_save(&p, 0x400100, 0x7fff00);
    trapframe_t snap = p.tf;
    clobber(&p);
    trap_restore(&p, &snap);
    printf("pid=%d ppid=%d state=%d pc=0x%lx sp=0x%lx r0=%ld r7=%ld root=0x%lx name=%s\n",
           p.pid, p.ppid, p.state, p.tf.pc, p.tf.sp,
           p.tf.regs[0], p.tf.regs[7], p.pageroot, p.name);
    return p.tf.pc != 0x400100 || p.tf.regs[7] != 107;
}

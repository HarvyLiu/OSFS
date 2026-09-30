// test_main.c -- 5 checks: save fidelity, restore after trash, isolation, fields.
#include <assert.h>
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

int main(void) {
    pcb_t a, b;
    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    a.pid = 1; a.ppid = 0; a.state = RUNNING; a.pageroot = 0x100000;
    for (int i = 0; i < NREG; i++) a.tf.regs[i] = 100 + i;
    a.tf.pc = 0x400100; a.tf.sp = 0x7fff00; a.tf.flags = 0x202;
    trapframe_t snap = a.tf;
    for (int i = 0; i < NREG; i++) a.tf.regs[i] = -1;  // trash
    a.tf = snap;                                        // restore
    assert(a.tf.pc == 0x400100 && a.tf.regs[7] == 107 && a.tf.flags == 0x202);
    b.pid = 2; b.ppid = 1; b.state = RUNNABLE;          // isolation: b untouched
    assert(b.tf.pc == 0 && b.pid == 2 && b.ppid == 1);
    assert(a.pageroot == 0x100000 && a.state == RUNNING);
    snprintf(a.name, 16, "%s", "demo");
    assert(strcmp(a.name, "demo") == 0);
    printf("all pcb-paper checks pass\n");
    return 0;
}

// rings.c -- THE rule: cpl <= dpl numerically. Lesson docs/en.md.
#include <stdio.h>

#define RING_KERNEL 0
#define RING_USER 3

typedef struct { const char *name; int dpl; int target; } gate_t;
typedef struct { const char *name; int dpl; } seg_t;

static int gate_call(int cpl, const gate_t *g) { return cpl <= g->dpl ? 0 : -1; }
static int seg_access(int cpl, const seg_t *s) { return cpl <= s->dpl ? 0 : -1; }

int main(void) {
    gate_t syscall_gate = {"syscall", 3, RING_KERNEL};
    gate_t konly_gate = {"kdebug", 0, RING_KERNEL};
    seg_t kdata = {"kdata", 0};
    seg_t udata = {"udata", 3};
    int v[5];
    v[0] = gate_call(RING_USER, &syscall_gate);
    v[1] = gate_call(RING_USER, &konly_gate);
    v[2] = seg_access(RING_USER, &kdata);
    v[3] = seg_access(RING_KERNEL, &udata);
    v[4] = seg_access(RING_USER, &udata);
    printf("syscall-gate=%s kdebug-gate=%s kdata=%s k-reads-u=%s u-reads-u=%s\n",
           v[0] == 0 ? "ok" : "FAULT", v[1] == 0 ? "ok" : "FAULT",
           v[2] == 0 ? "ok" : "FAULT", v[3] == 0 ? "ok" : "FAULT",
           v[4] == 0 ? "ok" : "FAULT");
    return !(v[0] == 0 && v[1] == -1 && v[2] == -1 && v[3] == 0 && v[4] == 0);
}

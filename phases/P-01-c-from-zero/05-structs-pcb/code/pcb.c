// pcb.c -- tiny process table. Lesson docs/en.md.
#include <stdio.h>
#include <string.h>

typedef enum { UNUSED = 0, RUNNABLE, RUNNING, ZOMBIE } state_t;

typedef struct {
    int pid;
    state_t state;
    unsigned long rsp;
    char name[16];
} pcb_t;

#define NPROC 4
static pcb_t table[NPROC];

static pcb_t *alloc_pid(int pid, const char *name) {
    for (int i = 0; i < NPROC; i++)
        if (table[i].state == UNUSED) {
            table[i].pid = pid;
            table[i].state = RUNNABLE;
            table[i].rsp = 0;
            snprintf(table[i].name, 16, "%s", name);
            return &table[i];
        }
    return 0;
}

static pcb_t *find_pid(int pid) {
    for (int i = 0; i < NPROC; i++)
        if (table[i].state != UNUSED && table[i].pid == pid)
            return &table[i];
    return 0;
}

int main(void) {
    pcb_t *a = alloc_pid(1, "init");
    pcb_t *b = alloc_pid(2, "shell");
    if (!a || !b) { fprintf(stderr, "table full\n"); return 1; }
    printf("sizeof(pcb_t)=%zu pid_off=%zu state_off=%zu\n",
           sizeof(pcb_t), __builtin_offsetof(pcb_t, pid),
           __builtin_offsetof(pcb_t, state));
    a->state = RUNNING;
    b->state = ZOMBIE;
    printf("run %d(%s) zombie %d(%s)\n", a->pid, a->name, b->pid, b->name);
    pcb_t *f = find_pid(2);
    if (f) { f->state = UNUSED; printf("reaped pid 2\n"); }
    printf("slots used=%d\n",
           (table[0].state != UNUSED) + (table[1].state != UNUSED) +
           (table[2].state != UNUSED) + (table[3].state != UNUSED));
    return 0;
}

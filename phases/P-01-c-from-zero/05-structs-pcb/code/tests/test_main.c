// test_main.c -- 5 checks: alloc, find, transitions, reap, full-table.
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef enum { UNUSED = 0, RUNNABLE, RUNNING, ZOMBIE } state_t;
typedef struct { int pid; state_t state; unsigned long rsp; char name[16]; } pcb_t;
#define NPROC 4
static pcb_t table[NPROC];

static pcb_t *alloc_pid(int pid, const char *name) {
    for (int i = 0; i < NPROC; i++)
        if (table[i].state == UNUSED) {
            table[i].pid = pid;
            table[i].state = RUNNABLE;
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
    assert(find_pid(99) == 0);              // empty table: miss
    pcb_t *a = alloc_pid(1, "init");
    pcb_t *b = alloc_pid(2, "shell");
    assert(a && b && a != b);
    a->state = RUNNING;
    assert(find_pid(1)->state == RUNNING);  // shared row, no copy
    b->state = ZOMBIE;
    find_pid(2)->state = UNUSED;            // reap
    assert(find_pid(2) == 0);
    alloc_pid(3, "a"); alloc_pid(4, "b"); alloc_pid(5, "c");
    assert(alloc_pid(6, "d") == 0);         // table full: honest NULL
    assert(sizeof(pcb_t) >= 28);            // 4+4+8+16 floor, padding above
    printf("all structs-pcb checks pass\n");
    return 0;
}

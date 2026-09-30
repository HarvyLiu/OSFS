// ctx.c -- cooperative round-robin with live rsp samples. Lesson docs/en.md.
#include <stdio.h>

unsigned long get_rsp(void);

typedef struct { int pid; int state; int step; unsigned long rsp; } pcb_t;
#define RUNNABLE 0
#define RUNNING 1

static int next_task(int cur, int n) { return (cur + 1) % n; }

static void run_slice(pcb_t *p) {
    p->state = RUNNING;
    p->rsp = get_rsp();
    printf("task %d step %d rsp=0x%lx\n", p->pid, p->step, p->rsp);
    p->step++;
    p->state = RUNNABLE;
}

int main(void) {
    pcb_t tasks[2] = {{1, RUNNABLE, 0, 0}, {2, RUNNABLE, 0, 0}};
    int cur = 0;
    for (int tick = 0; tick < 6; tick++) {
        run_slice(&tasks[cur]);
        cur = next_task(cur, 2);
    }
    printf("done steps=%d,%d\n", tasks[0].step, tasks[1].step);
    return 0;
}

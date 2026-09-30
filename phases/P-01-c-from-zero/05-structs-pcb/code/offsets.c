// offsets.c -- field offset probe. Lesson docs/en.md.
#include <stdio.h>
#include <stddef.h>

typedef enum { UNUSED = 0, RUNNABLE, RUNNING, ZOMBIE } state_t;
typedef struct { int pid; state_t state; unsigned long rsp; char name[16]; } pcb_t;

int main(void) {
    printf("pid=%zu state=%zu rsp=%zu name=%zu size=%zu\n",
           offsetof(pcb_t, pid), offsetof(pcb_t, state),
           offsetof(pcb_t, rsp), offsetof(pcb_t, name), sizeof(pcb_t));
    return 0;
}

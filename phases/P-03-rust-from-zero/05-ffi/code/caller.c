// caller.c -- the C side of the fence. Compiled (-c) only on host:
// real linking needs ONE matching target (kernel build), not MSVC+MinGW mixed.
// Lesson docs/en.md.
#include <stdint.h>

struct PcbFfi {
    int32_t pid;
    int32_t state;
    uint64_t rsp;
};

#define PCB_STATE_RUNNING 2

extern int64_t kern_add(int64_t a, int64_t b);

int call_it(void) {
    struct PcbFfi p;
    p.pid = 1;
    p.state = PCB_STATE_RUNNING;
    p.rsp = 0;
    return (int)(kern_add(40, 2) == 42) + p.pid;
}

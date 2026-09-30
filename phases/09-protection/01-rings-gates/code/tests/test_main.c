// test_main.c -- 5 checks: matrix, boundary equality, kernel omnipotence, DPL-2 room.
#include <assert.h>
#include <stdio.h>

static int gate_call(int cpl, int dpl) { return cpl <= dpl ? 0 : -1; }
static int seg_access(int cpl, int dpl) { return cpl <= dpl ? 0 : -1; }

int main(void) {
    assert(gate_call(3, 3) == 0);   // syscall gate: equality opens
    assert(gate_call(3, 0) == -1);  // kernel gate: user faults
    assert(seg_access(3, 0) == -1); // kdata from user: faults
    assert(seg_access(0, 3) == 0 && seg_access(0, 0) == 0);  // kernel: all open
    assert(gate_call(3, 2) == -1);  // DPL-2 room still enforces
    assert(seg_access(3, 3) == 0);  // own level: open
    printf("all rings-gates checks pass\n");
    return 0;
}

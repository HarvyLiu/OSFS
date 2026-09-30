// test_main.c -- framed add math + stack grows down. Links frame.s + rsp.s.
#include <assert.h>
#include <stdio.h>

int frame_add(int a, int b);
unsigned long get_rsp(void);

static unsigned long inner(void) {
    char pad[64];
    (void)pad;
    return get_rsp();
}

static unsigned long outer(void) {
    char pad[64];
    (void)pad;
    unsigned long a = get_rsp();
    unsigned long b = inner();
    assert(b < a);  // deeper call = lower address
    return a;
}

int main(void) {
    assert(frame_add(30, 12) == 42);
    assert(frame_add(0, 0) == 0);
    assert(frame_add(-5, 5) == 0);
    (void)outer();
    printf("all stack-frame checks pass\n");
    return 0;
}

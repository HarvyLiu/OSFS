// test_main.c -- 4 checks on the three addressing shapes. Links addr.s.
#include <assert.h>
#include <stdio.h>

int scaled_get(int *b, int i);
int mul3add(int x);
int field_state(void *p);

struct rec { int pid; int state; };

int main(void) {
    int arr[4] = {10, 20, 30, 40};
    assert(scaled_get(arr, 0) == 10);
    assert(scaled_get(arr, 3) == 40);
    assert(mul3add(7) == 21);
    assert(mul3add(-4) == -12);
    struct rec p = {5, 2};
    assert(field_state(&p) == 2);
    printf("all lea-addressing checks pass\n");
    return 0;
}

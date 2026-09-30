// main.c — P-01/03 pointers simulator.
// Lesson: phases/P-01-c-from-zero/03-c-pointers/docs/en.md
// Hosted C only (printf/malloc). Bare-metal kernels reimplement these.
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int x = 42;
    int *p = &x;
    printf("x=%d *p=%d\n", x, *p);
    *p = 99;
    printf("after *p=99: x=%d\n", x);
    printf("p points at %p, next int would be %p\n",
           (void *)p, (void *)(p + 1));

    int *q = malloc(sizeof(int));
    if (!q) return 1;
    *q = 123;
    printf("heap int=%d\n", *q);
    free(q);
    return 0;
}

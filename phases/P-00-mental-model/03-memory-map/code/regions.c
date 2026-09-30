// regions.c -- one resident per region + two lifetime transfers. Lesson docs/en.md.
// NOTE: make_mistake intentionally returns a stack address (-Wno-error keeps the
// warning visible as the lesson; NEVER ship this pattern).
#include <stdio.h>
#include <stdlib.h>

int g_init = 41;
int g_zero;

int *make_heap(void) {
    int *p = malloc(sizeof *p);
    if (p) *p = 7;
    return p;
}

int *make_mistake(void) {
    int local = 9;
    return &local;  // BUG on purpose (warning expected!)
}

int main(void) {
    int stack_var = 1;
    static int s_persistent = 2;
    int *h = make_heap();
    printf("code=%p g_init=%p g_zero=%p heap=%p stack=%p static-fn=%p\n",
           (void *)main, (void *)&g_init, (void *)&g_zero, (void *)h,
           (void *)&stack_var, (void *)&s_persistent);
    printf("heap-val=%d s_persistent=%d\n", h ? *h : -1, s_persistent);
    free(h);
    printf("mistake-ptr=%p (dangling: NEVER dereference)\n", (void *)make_mistake());
    return h == 0;
}

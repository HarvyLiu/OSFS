// test_main.c -- 5 checks: region rules without touching the dangling pointer.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int g_init = 41;
int g_zero;

int main(void) {
    assert(g_init == 41 && g_zero == 0);  // statics: born set/zeroed
    int stack_var = 1;
    static int s_persistent = 2;
    assert(stack_var == 1 && s_persistent == 2);
    int *h = malloc(sizeof *h);
    assert(h != 0);
    *h = 7;
    assert(*h == 7);                       // heap survives (owned)
    assert((void *)h != (void *)&stack_var);  // distinct neighborhoods
    assert((void *)&g_init != (void *)&g_zero);
    free(h);
    s_persistent = 5;                      // function-static persists...
    assert(s_persistent == 5);             // ...within this run (lifetime!)
    printf("all memory-map checks pass\n");
    return 0;
}

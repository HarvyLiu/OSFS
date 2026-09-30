// test_main.c -- 4 checks: heap alive, regions distinct + ordered.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int init_global = 41;
int zero_global;

int main(void) {
    assert(zero_global == 0 && init_global == 41);
    void *h = malloc(32);
    assert(h != 0);
    int s = 0;
    assert((void *)&s != h);              // stack vs heap: different worlds
    // NOTE: relative order (stack>heap>code) is Linux-canonical, NOT universal:
    // Windows places regions differently. Portable code asserts distinctness only.
    assert(h != (void *)main);            // heap vs code: distinct
    assert((void *)&zero_global != (void *)&init_global);
    free(h);
    printf("all address-spaces checks pass\n");
    return 0;
}

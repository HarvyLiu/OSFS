// test_main.c — 3 checks for pointer semantics. Run: cc test_main.c -o /tmp/t && /tmp/t
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

int main(void) {
    int x = 1;
    int *p = &x;
    assert(*p == 1);          // deref reads
    *p = 2;
    assert(x == 2);           // write-through modifies original
    int *a = malloc(sizeof(int) * 3);
    assert(a != 0);
    a[0] = 10; a[1] = 20; a[2] = 30;
    assert(*(a + 2) == a[2]); // [] is sugar for *(p+i)
    assert((void *)(a + 1) == (void *)&a[1]); // stride = sizeof(int)
    free(a);
    printf("all pointer checks pass\n");
    return 0;
}

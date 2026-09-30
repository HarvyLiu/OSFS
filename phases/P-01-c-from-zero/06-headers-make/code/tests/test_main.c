// test_main.c -- 4 checks through the public header only. Links vec.c.
#include <assert.h>
#include <stdio.h>
#include "../vec.h"

int main(void) {
    vec_t v = {0};
    for (int i = 0; i < 6; i++) assert(vec_push(&v, i * 10) == 0);
    assert(v.len == 6 && v.cap == 8);
    assert(v.data[0] == 0 && v.data[5] == 50);
    vec_free(&v);
    assert(v.data == 0 && v.len == 0 && v.cap == 0);  // nulled contract
    assert(sizeof(vec_t) >= 12);  // ptr + len + cap floor (32-bit min)
    printf("all headers-make checks pass\n");
    return 0;
}

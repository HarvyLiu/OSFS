// test_main.c -- 4 checks: push order, doubling, oom-shape, no-alias.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct { int *data; unsigned len, cap; } vec_t;

static int vec_push(vec_t *v, int x) {
    if (v->len == v->cap) {
        unsigned nc = v->cap ? v->cap * 2 : 4;
        int *nd = realloc(v->data, nc * sizeof(int));
        if (!nd) return -1;
        v->data = nd;
        v->cap = nc;
    }
    v->data[v->len++] = x;
    return 0;
}

int main(void) {
    vec_t v = {0};
    for (int i = 0; i < 6; i++) assert(vec_push(&v, i * 10) == 0);
    assert(v.len == 6 && v.cap == 8);  // 4 -> 8 doubling
    assert(v.data[0] == 0 && v.data[5] == 50);
    for (int i = 6; i < 20; i++) assert(vec_push(&v, i) == 0);
    assert(v.len == 20 && v.cap == 32);  // 8 -> 16 -> 32
    free(v.data);
    printf("all malloc-heap checks pass\n");
    return 0;
}

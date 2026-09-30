// heap.c -- growable int vector on raw malloc/realloc/free. Lesson docs/en.md.
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
    for (int i = 0; i < 6; i++)
        if (vec_push(&v, i * 10) != 0) { fprintf(stderr, "oom\n"); return 1; }
    printf("len=%u cap=%u vals=%d %d %d...\n", v.len, v.cap, v.data[0], v.data[1], v.data[2]);
    free(v.data);
    v.data = 0;
    return 0;
}

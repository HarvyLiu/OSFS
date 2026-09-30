// vec.c -- delivery of vec.h promises. Lesson docs/en.md.
#include <stdlib.h>
#include "vec.h"

int vec_push(vec_t *v, int x) {
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

void vec_free(vec_t *v) {
    free(v->data);
    v->data = 0;
    v->len = v->cap = 0;
}

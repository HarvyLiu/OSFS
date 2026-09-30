// main.c -- user of vec.h promises only. Lesson docs/en.md.
#include <stdio.h>
#include "vec.h"

int main(void) {
    vec_t v = {0};
    for (int i = 0; i < 6; i++)
        if (vec_push(&v, i * 10) != 0) return 1;
    printf("len=%u cap=%u first=%d last=%d\n", v.len, v.cap, v.data[0], v.data[5]);
    vec_free(&v);
    return 0;
}

// vec.h -- promises + guards. Lesson docs/en.md.
#ifndef OSFS_VEC_H
#define OSFS_VEC_H

typedef struct { int *data; unsigned len, cap; } vec_t;
int vec_push(vec_t *v, int x);
void vec_free(vec_t *v);

#endif

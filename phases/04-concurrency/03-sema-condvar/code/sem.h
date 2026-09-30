// sem.h -- counting semaphore shape. Lesson docs/en.md.
#ifndef OSFS_SEM_H
#define OSFS_SEM_H

#include <pthread.h>

typedef struct { int count; pthread_mutex_t m; pthread_cond_t cv; } csem_t;
void csem_init(csem_t *s, int v);
void csem_wait(csem_t *s);
void csem_post(csem_t *s);

#endif

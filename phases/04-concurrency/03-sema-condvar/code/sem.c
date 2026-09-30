// sem.c -- sleep instead of spin. Lesson docs/en.md.
#include "sem.h"

void csem_init(csem_t *s, int v) {
    s->count = v;
    pthread_mutex_init(&s->m, 0);
    pthread_cond_init(&s->cv, 0);
}

void csem_wait(csem_t *s) {
    pthread_mutex_lock(&s->m);
    while (s->count == 0)
        pthread_cond_wait(&s->cv, &s->m);
    s->count--;
    pthread_mutex_unlock(&s->m);
}

void csem_post(csem_t *s) {
    pthread_mutex_lock(&s->m);
    s->count++;
    pthread_cond_signal(&s->cv);
    pthread_mutex_unlock(&s->m);
}

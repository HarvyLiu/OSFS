// pc.c -- 200 items through 8 slots, zero loss. Lesson docs/en.md.
#include <stdio.h>
#include <pthread.h>
#include "sem.h"

#define NBUF 8
#define NITEM 200

static int buf[NBUF], in = 0, out = 0;
static pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;
static csem_t empty, full;

static void *producer(void *arg) {
    (void)arg;
    for (int i = 0; i < NITEM; i++) {
        csem_wait(&empty);
        pthread_mutex_lock(&mx);
        buf[in] = i;
        in = (in + 1) % NBUF;
        pthread_mutex_unlock(&mx);
        csem_post(&full);
    }
    return 0;
}

static void *consumer(void *arg) {
    long *sum = arg;
    for (int i = 0; i < NITEM; i++) {
        int v;
        csem_wait(&full);
        pthread_mutex_lock(&mx);
        v = buf[out];
        out = (out + 1) % NBUF;
        pthread_mutex_unlock(&mx);
        csem_post(&empty);
        *sum += v;
    }
    return 0;
}

int main(void) {
    pthread_t p, c;
    long sum = 0;
    csem_init(&empty, NBUF);
    csem_init(&full, 0);
    pthread_create(&p, 0, producer, 0);
    pthread_create(&c, 0, consumer, &sum);
    pthread_join(p, 0);
    pthread_join(c, 0);
    long want = (long)NITEM * (NITEM - 1) / 2;
    printf("consumed sum=%ld want=%ld\n", sum, want);
    return sum != want;
}

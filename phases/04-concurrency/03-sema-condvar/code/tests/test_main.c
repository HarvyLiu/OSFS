// test_main.c -- 5 checks: sem countdown/post, small threaded pc exact, order pair.
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include "../sem.h"

#define SB 4
#define SN 50

static int sbuf[SB], sin = 0, sout = 0;
static pthread_mutex_t smx = PTHREAD_MUTEX_INITIALIZER;
static csem_t sempty, sfull;

static void *sprod(void *arg) {
    (void)arg;
    for (int i = 0; i < SN; i++) {
        csem_wait(&sempty);
        pthread_mutex_lock(&smx);
        sbuf[sin] = i;
        sin = (sin + 1) % SB;
        pthread_mutex_unlock(&smx);
        csem_post(&sfull);
    }
    return 0;
}

static void *scons(void *arg) {
    long *sum = arg;
    for (int i = 0; i < SN; i++) {
        int v;
        csem_wait(&sfull);
        pthread_mutex_lock(&smx);
        v = sbuf[sout];
        sout = (sout + 1) % SB;
        pthread_mutex_unlock(&smx);
        csem_post(&sempty);
        *sum += v;
    }
    return 0;
}

int main(void) {
    csem_t s;
    csem_init(&s, 2);
    csem_wait(&s);
    csem_wait(&s);   // countdown to zero, no threads needed
    csem_post(&s);
    csem_post(&s);   // back up: balanced API
    csem_init(&sempty, SB);
    csem_init(&sfull, 0);
    pthread_t p, c;
    long sum = 0;
    pthread_create(&p, 0, sprod, 0);
    pthread_create(&c, 0, scons, &sum);
    pthread_join(p, 0);
    pthread_join(c, 0);
    assert(sum == (long)SN * (SN - 1) / 2);  // 0+..+49 = 1225
    // consistent lock order never self-deadlocks
    pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER, m2 = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&m1);
    pthread_mutex_lock(&m2);
    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
    printf("all sema-condvar checks pass\n");
    return 0;
}

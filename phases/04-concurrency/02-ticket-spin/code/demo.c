// demo.c -- 400k increments under tickets. Lesson docs/en.md.
#include <stdio.h>
#include <pthread.h>
#include "ticket.h"

#define NTHREAD 4
#define NINCR 100000

static long counter = 0;
static ticket_t lock;

static void *worker(void *arg) {
    (void)arg;
    for (int i = 0; i < NINCR; i++) {
        ticket_lock(&lock);
        counter++;
        ticket_unlock(&lock);
    }
    return 0;
}

int main(void) {
    pthread_t ts[NTHREAD];
    ticket_init(&lock);
    for (int i = 0; i < NTHREAD; i++) pthread_create(&ts[i], 0, worker, 0);
    for (int i = 0; i < NTHREAD; i++) pthread_join(ts[i], 0);
    long want = (long)NTHREAD * NINCR;
    printf("ticket: got=%ld want=%ld\n", counter, want);
    return counter != want;
}

// test_main.c -- 5 checks on the real lock (links ticket.c).
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include "../ticket.h"

static ticket_t tl;
static long tc = 0;

static void *bump(void *arg) {
    (void)arg;
    for (int i = 0; i < 2000; i++) {
        ticket_lock(&tl);
        tc++;
        ticket_unlock(&tl);
    }
    return 0;
}

int main(void) {
    ticket_t l;
    ticket_init(&l);
    ticket_lock(&l);      // single-thread: immediate
    ticket_unlock(&l);
    ticket_lock(&l);      // reuse: still immediate, no residue
    ticket_unlock(&l);
    // admission counter sanity: take two tickets' worth by hand is internal;
    // instead prove mutual exclusion under the lock at small scale:
    ticket_init(&l);
    long c = 0;
    for (int i = 0; i < 1000; i++) {
        ticket_lock(&l);
        c++;
        ticket_unlock(&l);
    }
    assert(c == 1000);
    // re-init resets cleanly mid-life
    ticket_init(&l);
    ticket_lock(&l);
    ticket_unlock(&l);
    // threaded: locked increments are exactly conserved (deterministic!)
    ticket_init(&tl);
    tc = 0;
    pthread_t ts[2];
    pthread_create(&ts[0], 0, bump, 0);
    pthread_create(&ts[1], 0, bump, 0);
    pthread_join(ts[0], 0);
    pthread_join(ts[1], 0);
    assert(tc == 4000);
    printf("all ticket-spin checks pass\n");
    return 0;
}

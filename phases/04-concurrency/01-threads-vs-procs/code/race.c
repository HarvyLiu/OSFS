// race.c -- racy vs mutex counter, same work. Lesson docs/en.md.
// Needs -pthread at compile AND link.
#include <stdio.h>
#include <pthread.h>

#define NTHREAD 4
#define NINCR 100000

static long counter = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void *racy(void *arg) {
    (void)arg;
    for (int i = 0; i < NINCR; i++) counter++;
    return 0;
}

static void *safe(void *arg) {
    (void)arg;
    for (int i = 0; i < NINCR; i++) {
        pthread_mutex_lock(&lock);
        counter++;
        pthread_mutex_unlock(&lock);
    }
    return 0;
}

static long run(void *(*fn)(void *)) {
    pthread_t ts[NTHREAD];
    counter = 0;
    for (int i = 0; i < NTHREAD; i++) pthread_create(&ts[i], 0, fn, 0);
    for (int i = 0; i < NTHREAD; i++) pthread_join(ts[i], 0);
    return counter;
}

int main(void) {
    long want = (long)NTHREAD * NINCR;
    long racy_got = run(racy);
    long safe_got = run(safe);
    printf("no-mutex: got=%ld want=%ld %s\n", racy_got, want,
           racy_got == want ? "(exact this run: lucky timing)" : "(short: lost updates)");
    printf("mutex: got=%ld want=%ld\n", safe_got, want);
    return safe_got != want;
}

// test_main.c -- deterministic checks: mutex path exact at two sizes + join contract.
#include <assert.h>
#include <stdio.h>
#include <pthread.h>

static long counter = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct { int n; } arg_t;

static void *worker(void *arg) {
    int n = ((arg_t *)arg)->n;
    for (int i = 0; i < n; i++) {
        pthread_mutex_lock(&lock);
        counter++;
        pthread_mutex_unlock(&lock);
    }
    return arg;  // echo pointer: join retrieves it
}

static long run_locked(int nthread, int nincr) {
    pthread_t ts[4];
    arg_t a = {nincr};
    counter = 0;
    for (int i = 0; i < nthread; i++) pthread_create(&ts[i], 0, worker, &a);
    for (int i = 0; i < nthread; i++) {
        void *ret = 0;
        pthread_join(ts[i], &ret);
        assert(ret == &a);  // join contract: worker's return arrives
    }
    return counter;
}

int main(void) {
    assert(run_locked(2, 1000) == 2000);
    assert(run_locked(4, 5000) == 20000);
    assert(run_locked(1, 100) == 100);  // single thread: trivially exact
    printf("all threads-races checks pass\n");
    return 0;
}

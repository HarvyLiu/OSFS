// order.c -- lock-order cure + trylock probe (never hangs). Lesson docs/en.md.
#include <stdio.h>
#include <pthread.h>

static pthread_mutex_t A = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t B = PTHREAD_MUTEX_INITIALIZER;

static int consistent(void) {
    pthread_mutex_lock(&A);
    pthread_mutex_lock(&B);
    pthread_mutex_unlock(&B);
    pthread_mutex_unlock(&A);
    return 0;
}

int main(void) {
    pthread_mutex_lock(&B);
    int took_a = (pthread_mutex_trylock(&A) == 0);
    if (took_a) pthread_mutex_unlock(&A);
    pthread_mutex_unlock(&B);
    consistent();
    printf("order-rule: consistent path ok; inverted try took_a=%d\n", took_a);
    return 0;
}

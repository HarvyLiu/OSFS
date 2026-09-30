// types.c — sizes on YOUR machine. Lesson docs/en.md.
#include <stdio.h>

int main(void) {
    char c = 'A';
    int n = -42;
    long big = 1000000L;
    printf("c=%c (%d) n=%d big=%ld sizes=%zu/%zu/%zu\n",
           c, c, n, big, sizeof(c), sizeof(n), sizeof(big));
    return 0;
}

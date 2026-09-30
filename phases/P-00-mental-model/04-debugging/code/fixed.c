// fixed.c -- the cure: NULL checked, loop bounded. Tests pin both fixes.
// Lesson P-00-mental-model/04-debugging/docs/en.md.
#include <stdio.h>
#include <string.h>

const char *lookup(const char *name) {
    if (strcmp(name, "amy") == 0) return "1001";
    if (strcmp(name, "bo") == 0) return "1002";
    return 0;
}

double average(int *a, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) sum += a[i];  // FIX: < not <=.
    return (double)sum / n;
}

#ifndef FIXED_TEST
int main(int argc, char **argv) {
    if (argc < 2) { printf("usage: fixed <name>\n"); return 1; }
    const char *id = lookup(argv[1]);
    if (!id) { printf("unknown name: %s\n", argv[1]); return 2; }  // FIX: check.
    printf("id=%s\n", id);
    int scores[3] = {90, 80, 70};
    printf("avg=%.1f\n", average(scores, 3));
    return 0;
}
#endif

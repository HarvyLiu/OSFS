// buggy.c -- the patient: builds clean, dies on input "bob". See fixed.c.
// Lesson P-00-mental-model/04-debugging/docs/en.md.
#include <stdio.h>
#include <string.h>

static const char *lookup(const char *name) {
    if (strcmp(name, "amy") == 0) return "1001";
    if (strcmp(name, "bo") == 0) return "1002";
    return 0;  // BUG 1: unknown name -> NULL, caller never checks.
}

static double average(int *a, int n) {
    int sum = 0;
    for (int i = 0; i <= n; i++) sum += a[i];  // BUG 2: <= overreads one.
    return (double)sum / n;
}

int main(int argc, char **argv) {
    if (argc < 2) { printf("usage: buggy <name>\n"); return 1; }
    const char *id = lookup(argv[1]);
    printf("id=%s (len=%zu)\n", id, strlen(id));  // BOOM when id == NULL.
    int scores[3] = {90, 80, 70};
    printf("avg=%.1f\n", average(scores, 3));
    return 0;
}

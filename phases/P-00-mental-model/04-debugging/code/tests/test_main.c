// test_main.c -- pins the two fixes (exit 0 = cured). Links fixed.c as lib.
// Lesson P-00-mental-model/04-debugging/docs/en.md.
#include <assert.h>
#include <stdio.h>
#include <string.h>

const char *lookup(const char *name);
double average(int *a, int n);

int main(void) {
    assert(lookup("amy") && strcmp(lookup("amy"), "1001") == 0);
    assert(lookup("nobody") == 0);          // NULL is a value: check it.
    int s[3] = {90, 80, 70};
    double avg = average(s, 3);
    assert(avg > 79.9 && avg < 80.1);       // <= bug gave ~103.3, not 80.
    printf("all debugging checks pass\n");
    return 0;
}

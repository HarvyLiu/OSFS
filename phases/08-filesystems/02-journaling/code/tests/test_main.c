// test_main.c -- 6 checks on the real log (links journal.c).
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../journal.h"

static void fillb(unsigned char *b, char c) { memset(b, c, J_BSIZE); }

int main(void) {
    journal_t j;
    unsigned char a[J_BSIZE], b[J_BSIZE];
    fillb(a, 'A');
    fillb(b, 'B');
    // fate 0: staged, never committed -> zeros, consistent
    j_init(&j);
    assert(j_write(&j, 3, a) == 0 && j_write(&j, 5, b) == 0);
    assert(j_crash(&j, 0) == 0);
    assert(j.home[3][0] == 0 && j.home[5][0] == 0);
    // fate 1: committed -> replay applies both
    j_init(&j);
    j_write(&j, 3, a);
    j_write(&j, 5, b);
    assert(j_commit(&j) == 0);
    assert(j_recover(&j) == 1);
    assert(j.home[3][0] == 'A' && j.home[5][0] == 'B');
    // idempotence: recover again -> same bytes, no-op
    assert(j_recover(&j) == 0);
    assert(j.home[3][0] == 'A' && j.home[5][0] == 'B');
    // guards: empty commit refused, over-capacity staged refused, bad bno refused
    j_init(&j);
    assert(j_commit(&j) == -1);
    assert(j_write(&j, 99, a) == -1);
    for (int i = 0; i < J_MAXPEND; i++) assert(j_write(&j, i, a) == 0);
    assert(j_write(&j, 0, a) == -1);
    printf("all journaling checks pass\n");
    return 0;
}

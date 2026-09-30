// demo.c -- three crashes, three fates. Lesson docs/en.md.
#include <stdio.h>
#include <string.h>
#include "journal.h"

static void fill(unsigned char *b, char c) { memset(b, c, J_BSIZE); }

static int run_case(int point) {
    journal_t j;
    j_init(&j);
    unsigned char a[J_BSIZE], b[J_BSIZE], out[J_BSIZE];
    fill(a, 'A');
    fill(b, 'B');
    j_write(&j, 3, a);
    j_write(&j, 5, b);
    if (point >= 1) j_commit(&j);
    if (point >= 2) j_checkpoint(&j);
    j_crash(&j, point);
    memcpy(out, j.home[3], J_BSIZE);
    int v3 = out[0];
    memcpy(out, j.home[5], J_BSIZE);
    int v5 = out[0];
    int ok = (v3 == (point == 0 ? 0 : 'A')) && (v5 == (point == 0 ? 0 : 'B'));
    printf("crash@%d: blk3=%c blk5=%c %s\n", point,
           v3 ? v3 : '-', v5 ? v5 : '-', ok ? "CONSISTENT" : "CORRUPT");
    return !ok;
}

int main(void) {
    int rc = 0;
    rc |= run_case(0);
    rc |= run_case(1);
    rc |= run_case(2);
    return rc;
}

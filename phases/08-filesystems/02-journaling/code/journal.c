// journal.c -- intent first, effects after. Lesson docs/en.md.
#include <string.h>
#include "journal.h"

void j_init(journal_t *j) {
    memset(j, 0, sizeof *j);
}

int j_write(journal_t *j, int bno, const unsigned char *data) {
    if (bno < 0 || bno >= J_NBLOCKS || j->npend >= J_MAXPEND) return -1;
    memcpy(j->jbuf[j->npend], data, J_BSIZE);
    j->jbno[j->npend] = bno;
    j->npend++;
    return 0;
}

int j_commit(journal_t *j) {
    if (j->npend == 0) return -1;
    j->committed = 1;
    return 0;
}

int j_checkpoint(journal_t *j) {
    for (int i = 0; i < j->npend; i++)
        memcpy(j->home[j->jbno[i]], j->jbuf[i], J_BSIZE);
    j->npend = 0;
    j->committed = 0;
    return 0;
}

int j_recover(journal_t *j) {
    if (!j->committed) { j->npend = 0; return 0; }
    for (int i = 0; i < j->npend; i++)
        memcpy(j->home[j->jbno[i]], j->jbuf[i], J_BSIZE);
    j->npend = 0;
    j->committed = 0;
    return 1;
}

int j_crash(journal_t *j, int point) {
    if (point == 0) {
        memset(j->jbuf, 0, sizeof j->jbuf);
        j->npend = 0;
        return 0;
    }
    if (point == 1) return j_recover(j);
    j_checkpoint(j);
    return j_recover(j);
}

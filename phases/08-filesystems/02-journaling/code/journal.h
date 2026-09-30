// journal.h -- write-ahead log shape. Lesson docs/en.md.
#ifndef OSFS_JOURNAL_H
#define OSFS_JOURNAL_H

#define J_NBLOCKS 16
#define J_BSIZE 32
#define J_MAXPEND 4

typedef struct {
    unsigned char home[J_NBLOCKS][J_BSIZE];
    unsigned char jbuf[J_MAXPEND][J_BSIZE];
    int jbno[J_MAXPEND];
    int npend;
    int committed;
} journal_t;

void j_init(journal_t *j);
int j_write(journal_t *j, int bno, const unsigned char *data);
int j_commit(journal_t *j);
int j_checkpoint(journal_t *j);
int j_recover(journal_t *j);
int j_crash(journal_t *j, int point);

#endif

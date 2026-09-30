// perms.c -- first-match Unix + capability gate. Lesson docs/en.md.
#include <stdio.h>

#define CAP_DISK 0x1u

typedef struct { int uid; int gid; unsigned caps; const char *who; } proc_t;
typedef struct { int owner; int group; unsigned mode; const char *name; } file_t;

static int unix_open(const proc_t *p, const file_t *f, int want_write) {
    if (p->uid == 0) return 0;
    unsigned bits;
    if (p->uid == f->owner) bits = (f->mode >> 6) & 7;
    else if (p->gid == f->group) bits = (f->mode >> 3) & 7;
    else bits = f->mode & 7;
    unsigned need = want_write ? 2 : 4;
    return (bits & need) ? 0 : -1;
}

static int raw_open(const proc_t *p) {
    if (p->uid != 0 && !(p->caps & CAP_DISK)) return -1;
    return 0;
}

int main(void) {
    file_t note = {1000, 100, 0640, "note"};
    proc_t alice = {1000, 100, 0, "alice"};
    proc_t bob = {101, 100, 0, "bob"};
    proc_t eve = {200, 200, 0, "eve"};
    proc_t root = {0, 0, 0, "root"};
    proc_t op = {200, 200, CAP_DISK, "op-with-cap"};
    int v[6];
    v[0] = unix_open(&alice, &note, 1);
    v[1] = unix_open(&bob, &note, 1);
    v[2] = unix_open(&bob, &note, 0);
    v[3] = unix_open(&eve, &note, 0);
    v[4] = unix_open(&root, &note, 1);
    v[5] = raw_open(&op);
    printf("alice-w=%s bob-w=%s bob-r=%s eve-r=%s root-w=%s op-raw=%s\n",
           v[0]==0?"ok":"DENY", v[1]==0?"ok":"DENY", v[2]==0?"ok":"DENY",
           v[3]==0?"ok":"DENY", v[4]==0?"ok":"DENY", v[5]==0?"ok":"DENY");
    int eve_raw = raw_open(&eve);
    printf("eve-raw=%s (no cap, non-root)\n", eve_raw==0?"ok":"DENY");
    return !(v[0]==0 && v[1]==-1 && v[2]==0 && v[3]==-1 && v[4]==0 && v[5]==0 && eve_raw==-1);
}

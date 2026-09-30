// test_main.c -- 6 checks: matrix, first-match order, root, caps, setuid-shape.
#include <assert.h>
#include <stdio.h>

#define CAP_DISK 0x1u

typedef struct { int uid; int gid; unsigned caps; } proc_t;
typedef struct { int owner; int group; unsigned mode; } file_t;

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
    file_t note = {1000, 100, 0640};
    proc_t alice = {1000, 100, 0}, bob = {101, 100, 0}, eve = {200, 200, 0};
    proc_t root = {0, 0, 0}, op = {200, 200, CAP_DISK};
    assert(unix_open(&alice, &note, 1) == 0);
    assert(unix_open(&bob, &note, 1) == -1);   // group r--: write denied
    assert(unix_open(&bob, &note, 0) == 0);    // group read: ok (first-match!)
    assert(unix_open(&eve, &note, 0) == -1);
    assert(unix_open(&root, &note, 1) == 0);   // skeleton key
    assert(raw_open(&op) == 0 && raw_open(&eve) == -1);  // cap, not identity
    // setuid shape: run-as file owner for one call
    proc_t borrowed = eve;
    borrowed.uid = note.owner;                // borrowed alice
    assert(unix_open(&borrowed, &note, 1) == 0);
    printf("all users-caps checks pass\n");
    return 0;
}

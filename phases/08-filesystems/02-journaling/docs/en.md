# Journaling + Crash Consistency — All of the Update, or None of It

> Write the intent first. Commit the intent. Then do it. Crash anywhere: replay or discard, never half.

**Type:** Build
**Languages:** C
**Prerequisites:** 01-vsfs
**College ref:** OSTEP Ch.42–43 (crash consistency + journaling), MIT 6.1810 logging labs (commit/recover), xv6 file kernel/log.c (log_write/commit/recover yours mirrors)
**Time:** ~90 minutes

## Learning Objectives
- Trace one 2-block atomic update through stage/commit/checkpoint with crashes at all 3 points
- Implement write-ahead log: stage → commit-flag → checkpoint → replay-on-recover in C
- Explain why commit is the point of no return (and what replays twice safely — idempotence)
- Connect journaling to VSFS blocks (the vocabulary) and to `fsck` (the alternative: scan-and-repair)

## Concept in 60s

![commit protocol](../figures/commit-protocol.svg)

<!-- source: ../figures/commit-protocol.excalidraw — open in excalidraw.com to redraw -->

Your file update touches two or three blocks at once — bitmap, inode, data, the VSFS trio — and a crash between those writes leaves the filesystem half-updated, which is corrupt. So you write the intent before the effect. First *stage* the new block contents into the journal area while home stays untouched. Then *commit* with a single flag write: this transaction is whole. Then *checkpoint*, copying the journaled blocks home, and free the transaction. Crash before the commit and you discard the journal — home was never touched, so nothing happened. Crash after it and you replay the journal home on reboot — redo to completion, so everything happened. Replay just copies bytes, so running it twice is the same as once: idempotent, and safe even if you crash mid-replay. One flag separates nothing from all.

## Simulate It (host C — journal over RAM blocks, portable)

Split `code/journal.h` + `code/journal.c` + `code/demo.c` (crash points injected, never accidental).

```c
// journal.h -- write-ahead log shape.
#ifndef OSFS_JOURNAL_H
#define OSFS_JOURNAL_H

#define J_NBLOCKS 16
#define J_BSIZE 32
#define J_MAXPEND 4

typedef struct {
    unsigned char home[J_NBLOCKS][J_BSIZE];   // the "filesystem"
    unsigned char jbuf[J_MAXPEND][J_BSIZE];   // staged copies
    int jbno[J_MAXPEND];                       // which home block each stages
    int npend;                                 // staged count
    int committed;                             // the flag: 0 nothing / 1 all
} journal_t;

void j_init(journal_t *j);
int j_write(journal_t *j, int bno, const unsigned char *data); // stage (0/-1)
int j_commit(journal_t *j);      // flag=1: point of no return
int j_checkpoint(journal_t *j);  // journal -> home, flag=0
int j_recover(journal_t *j);     // replay iff committed (idempotent)
int j_crash(journal_t *j, int point); // 0 pre-commit / 1 post-commit / 2 post-checkpoint

#endif
```

What this does: publishes the four-phase protocol with crash injection as a *verb* (deterministic disasters, scheduled not hoped-for).

```c
// journal.c -- intent first, effects after.
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
    return 0; // home untouched: staging only (the rule)
}

int j_commit(journal_t *j) {
    if (j->npend == 0) return -1;
    j->committed = 1;   // ONE flag: nothing -> everything
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
    if (!j->committed) { j->npend = 0; return 0; } // discard: nothing happened
    for (int i = 0; i < j->npend; i++)
        memcpy(j->home[j->jbno[i]], j->jbuf[i], J_BSIZE);
    j->npend = 0;
    j->committed = 0;
    return 1; // replayed: everything happened (idempotent: run again = same)
}

int j_crash(journal_t *j, int point) {
    if (point == 0) {          // pre-commit: journal dies with RAM
        memset(j->jbuf, 0, sizeof j->jbuf);
        j->npend = 0;          // committed already 0: home intact
        return 0;
    }
    if (point == 1) return j_recover(j);  // reboot path: replay iff committed
    j_checkpoint(j);           // post-checkpoint: already home (replay harmless)
    return j_recover(j);
}
```

What this does: stages without touching home, commits one flag, checkpoints copies, recovers by flag — with crashes as scheduled calls (determinism over drama).

| Lines | Code | Why it exists |
|---|---|---|
| 12–20 | stage-only writes | home untouched until checkpoint (the write-ahead invariant: effects follow intent, never precede) |
| 22–26 | commit flag | one integer = the atomic point (flags write atomically in our model; real journals checksum/commit-block for the same guarantee on real disks) |
| 28–34 | checkpoint | copy-then-clear (home updated *after* commit exists — ordering is the protocol) |
| 36–42 | recover branches | uncommitted → discard (nothing happened, provably); committed → replay (everything happens, idempotently — run twice, same bytes) |
| 44–53 | crash points | 0 = amnesia of intent, 1 = reboot replay, 2 = belt-and-suspenders (checkpoint then recover-noop — safe by idempotence) |

Change X → Y: checkpoint *before* commit (swap the calls in a scratch demo). Verify: crash between = home half-new with no journal to repair (the exact corruption journals exist to forbid — order *is* the feature).

```c
// demo.c -- three crashes, three fates.
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
    j_crash(&j, point);   // scheduled disaster
    memcpy(out, j.home[3], J_BSIZE);
    int v3 = out[0];
    memcpy(out, j.home[5], J_BSIZE);
    int v5 = out[0];
    int ok = (v3 == (point == 0 ? 0 : 'A')) && (v5 == (point == 0 ? 0 : 'B'));
    printf("crash@%d: blk3=%c blk5=%c %s\n", point,
           v3 ? v3 : '-', v5 ? v5 : '-', ok ? "CONSISTENT" : "CORRUPT");
    return !ok;
}
```

What this does: stages A+B, then crashes pre-commit (zeros, consistent), post-commit (replay → A/B, consistent), post-checkpoint (A/B, consistent) — all-or-nothing at all three fates.

| Lines | Code | Why it exists |
|---|---|---|
| `point >= 1/2` gates | phase control | crash@N runs phases <N (fate selected by how far the protocol got — the matrix under test) |
| `v3 ? v3 : '-'` | printable fate | 0 prints as `-` (zero isn't a character — display honesty for byte checks) |
| `rc |=` accumulation | all fates must pass | one corrupt fate fails the binary (consistency is universal, not majority) |

Change X → Y: comment out the `j_commit` gate (never commit, crash@1). Verify: `blk3=- blk5=- CONSISTENT` (uncommitted replay discards — the "nothing happened" branch, now observed instead of trusted).

## Build It

```bash
make run
make test
```

What this does: prints three CONSISTENT fates, then asserts discard/replay/idempotence/torn-forbidding on the real `journal.c`.

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | three lines, all CONSISTENT, fates 0/1/2 (zeros, replayed, checkpointed) |
| `make test` | machine proof | recover-twice idempotence + partial-journal discard + real-code linkage |

Change X → Y: `j_commit` twice (double commit). Verify: still consistent (flag is boolean, replay copies same bytes — commit idempotent too).

## Use It (Linux)

Real journals keep the same diary:

```bash
dmesg 2>/dev/null | grep -iE "journal|recovery|recover" | head -4 || echo "(no journal lines: clean mounts leave quiet diaries)"
tune2fs -l $(df / --output=source 2>/dev/null | tail -1) 2>/dev/null | grep -iE "journal|features" | head -4 || echo "(need extN device rights; the toy above stands alone)"
```

What this does: looks for recovery lines (crash replays, production edition) and journal features (ordered/writeback modes — policy knobs for our commit/checkpoint split).

| Lines | Code | Why it exists |
|---|---|---|
| `dmesg journal` | recovery diary | `recovery complete` lines = `j_recover` at datacenter scale (same branch, real disks) |
| `tune2fs` | mode knobs | `has_journal`, `journal_data*` (what gets journaled: metadata-only vs data — our toy journals all, ext4 usually metadata) |

Change X → Y: `mount | grep -E "data=" | head -2`. Verify: `data=ordered` typical (mode names the commit/checkpoint *ordering* — vocabulary from this lesson, live).

## Ship It

Artifact: `outputs/journal-card.md` — stage/commit/checkpoint/recover verbs, crash-point matrix (0 discard / 1 replay / 2 noop), idempotence rule, torn-write definition. Crash-consistency pocket reference: VSFS card's blocks + this card's protocol = durable files. You now own all-or-nothing updates — FFS in 08/03 decides where those blocks should live.

## Exercises

1. Easy — crash@1 twice in a row (recover, recover again): same bytes (idempotence demonstrated, not asserted — then assert it in tests).
2. Medium — torn-write simulator: journal capacity 1 slot, stage 2 blocks, "crash" mid-stage (only first staged): recover → first applied, second absent — then argue why *commit-atomicity* ( Exercise: whole-txn commit blocks) forbids shipping this state (the torn-journal problem journals solve with checksummed commit records).
3. Hard — `fsck` alternative: corruptor flips random home bytes (no journal); writer checker scans bitmap-vs-inode agreement (VSFS structures!) and repairs; compare recovery *time* vs journal replay (scan-all vs replay-few — the tradeoff that picked journals for big disks).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| inode | journaled cargo (with bitmap+data: the atomic set) | [inode](../../../../glossary/terms.md#inode) |
| journal | intent log: stage → commit → checkpoint → replay | [journal](../../../../glossary/terms.md#journal) |
| syscall | `fsync` forces commit-to-platter (the durability verb beneath) | [syscall](../../../../glossary/terms.md#syscall) |
| address | journal + home are block-numbered spaces (offsets by convention) | [address](../../../../glossary/terms.md#address) |

## Further Reading

- OSTEP Ch.42–43 — crash consistency + journaling (this lesson runs its protocol).
- xv6 `kernel/log.c` — `begin_op/write/commit/recover` (production 200 lines of exactly this).
- `man 8 tune2fs` + `man 5 ext4` (journal modes — policy knobs for the protocol).

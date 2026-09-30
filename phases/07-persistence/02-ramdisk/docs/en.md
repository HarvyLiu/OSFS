# RAMdisk — A Disk Made of Forgetting Stuff (on Purpose)

> Same block verbs, no platters. RAM is fast, volatile, and the perfect filesystem laboratory.

**Type:** Build
**Languages:** C
**Prerequisites:** 01-block-layer
**College ref:** OSTEP Ch.36 (RAM as the fastest disk), Linux `brd` driver (ramdisk blocks), xv6 `mkfs` + `balloc` (RAM-image roots)
**Time:** ~75 minutes

## Learning Objectives
- Trace identical read/write/flush/crash calls against RAM vs file backends, comparing counters
- Implement a malloc-backed block device exposing 07/01's verb shape (init/read/write/flush/crash)
- Explain why RAMdisks exist (initramfs, test beds, speed) and why flush/crash mean less there
- Connect backend-swapping to interface design (same verbs, different durability — FFI-fence thinking from P-03/05)

## Concept in 60s

![ramdisk backends](../figures/ramdisk-backends.svg)

<!-- source: ../figures/ramdisk-backends.excalidraw — open in excalidraw.com to redraw -->

07/01's device backed blocks with a *file* (durable, slow-ish, crash = reload file). A RAMdisk backs them with *malloc'd RAM* (fast, volatile: process exit = total amnesia). Same verbs, same block numbers, same callers — VSFS next lesson won't know which it's on (that's the point: filesystems program to the *interface*, not the medium). Flush on RAM = memcpy to... itself (no-op with a counter bump — durability theater, honestly labeled). Crash on RAM = wipe (or reload from an optional snapshot file — our `blk_crash` semantic, RAM-speed). Real uses: initramfs (kernel boots from RAM before disks wake), FS test beds (this course's 08 phase runs here first).

## Simulate It (host C — RAM backend, portable)

Split `code/ramdisk.h` + `code/ramdisk.c` + `code/demo.c` (same verb shape as `blk.h`, new backend).

```c
// ramdisk.h -- same verbs, RAM behind.
#ifndef OSFS_RAMDISK_H
#define OSFS_RAMDISK_H

#include <stddef.h>

#define RD_NBLOCKS 64
#define RD_BSIZE 64

typedef struct {
    unsigned char *store;   // NBLOCKS*BSIZE bytes, malloc'd
    int dirty[RD_NBLOCKS];
    long reads, writes, flushes, drops;
    int live;               // 0 after crash-wipe (device gone dark)
} ramdisk_t;

int rd_init(ramdisk_t *d);
int rd_read(ramdisk_t *d, int bno, unsigned char *out);
int rd_write(ramdisk_t *d, int bno, const unsigned char *in);
int rd_flush(ramdisk_t *d);   // no-op durability theater (counts!)
int rd_crash(ramdisk_t *d);   // wipe all (volatile truth)
void rd_free(ramdisk_t *d);

#endif
```

What this does: publishes 07/01's contract with RAM semantics — `flush` honest no-op, `crash` total wipe (compare `blk_crash`'s reload: file remembers, RAM doesn't).

```c
// ramdisk.c -- malloc platters.
#include <stdlib.h>
#include <string.h>
#include "ramdisk.h"

int rd_init(ramdisk_t *d) {
    memset(d, 0, sizeof *d);
    d->store = calloc(RD_NBLOCKS, RD_BSIZE); // zeroed: fresh factory disk
    if (!d->store) return -1;
    d->live = 1;
    return 0;
}

int rd_read(ramdisk_t *d, int bno, unsigned char *out) {
    if (!d->live || bno < 0 || bno >= RD_NBLOCKS) return -1;
    memcpy(out, d->store + (size_t)bno * RD_BSIZE, RD_BSIZE);
    d->reads++;
    return 0;
}

int rd_write(ramdisk_t *d, int bno, const unsigned char *in) {
    if (!d->live || bno < 0 || bno >= RD_NBLOCKS) return -1;
    memcpy(d->store + (size_t)bno * RD_BSIZE, in, RD_BSIZE);
    d->dirty[bno] = 1;
    d->writes++;
    return 0;
}

int rd_flush(ramdisk_t *d) {
    if (!d->live) return -1;
    for (int i = 0; i < RD_NBLOCKS; i++) d->dirty[i] = 0;
    d->flushes++;
    return 0; // theater: nothing below to push to (documented, counted)
}

int rd_crash(ramdisk_t *d) {
    if (!d->live) return -1;
    memset(d->store, 0, sizeof(unsigned char) * RD_NBLOCKS * RD_BSIZE);
    memset(d->dirty, 0, sizeof d->dirty);
    d->drops++;
    return 0;
}

void rd_free(ramdisk_t *d) {
    free(d->store);
    d->store = 0;
    d->live = 0;
}
```

What this does: block storage as pointer arithmetic — write dirties, flush clears flags (no I/O!), crash zeroes everything (volatility, the feature).

| Lines | Code | Why it exists |
|---|---|---|
| 7–12 | `calloc` init | zeroed factory disk (`calloc` = malloc+zero — fresh platters have no previous tenant's data, security's first demand) |
| 15–19 | offset math | `store + bno*BSIZE` (P-02/03's scaled indexing, `size_t` so 64-bit clean — the addressing lesson paying rent) |
| 28–33 | flush theater | clears dirties, counts, moves *zero bytes* (honest comment required: RAM durability is oxymoronic — the counter records invocations, not I/O) |
| 35–42 | crash wipe | zeroes all (file-backend reloads truth; RAM has no truth below — total amnesia, the volatility contract) |
| 44–48 | free + `live=0` | process-exit preview: post-free calls fail `-1` (use-after-free at device scale — P-01/04's trilogy, one level up) |

Change X → Y: snapshot before crash (memcpy store aside, restore after). Verify: bytes survive "crash" (you just built persistence *above* volatility — that's what flush-to-file means, reimplemented in 3 lines).

```c
// demo.c -- same script both backends would play.
#include <stdio.h>
#include <string.h>
#include "ramdisk.h"

int main(void) {
    ramdisk_t d;
    if (rd_init(&d) != 0) return 1;
    unsigned char b[RD_BSIZE], out[RD_BSIZE];
    memset(b, 'R', sizeof b);
    rd_write(&d, 10, b);
    rd_read(&d, 10, out);
    printf("pre-crash=%d (82=R)\n", out[0]);
    rd_crash(&d);
    rd_read(&d, 10, out);
    printf("post-crash=%d (0=wiped) reads=%ld writes=%ld flushes=%ld drops=%ld\n",
           out[0], d.reads, d.writes, d.flushes, d.drops);
    rd_free(&d);
    return out[0] != 0;
}
```

What this does: writes `R`, proves coherence pre-crash (82), wipes, proves amnesia (0) — 07/01's diptych replayed where flush *can't* save you.

| Lines | Code | Why it exists |
|---|---|---|
| 12–14 | coherence check | RAM reads see writes instantly (same as file-backend — coherence is backend-blind) |
| 15–17 | amnesia check | 0 after wipe (file-backend showed *old* truth here — the backends diverge exactly at crash, nowhere else) |

Change X → Y: `rd_flush` before `rd_crash`, rerun. Verify: *still* 0 (the theater exposed — flush moves nothing on RAM; durability needs a medium below, which is the whole lesson).

## Build It

```bash
make run
make test
```

What this does: runs the amnesia diptych, then asserts roundtrip, wipe-loss, post-free failure, OOB bounds, flush-noop.

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | expect `pre-crash=82`, `post-crash=0` (coherence yes, durability no) |
| `make test` | machine proof | backend contract pinned (VSFS next lesson codes against *these* verbs) |

Change X → Y: point the demo at block 63 (last). Verify: works (bounds are `[0,64)` — edge blocks count; off-by-one hunts live here).

## Use It (Linux)

Real RAMdisks ship with the kernel:

```bash
ls /dev/ram* 2>/dev/null | head -3 || echo "(no brd loaded: modprobe brd for /dev/ram0)"
df -h /dev/shm 2>/dev/null | tail -1
mount | grep -E "tmpfs|ramfs" | head -3
```

What this does: looks for kernel RAM block devices, shows tmpfs (page-cache-backed "disk"), and RAM-backed mounts — volatility products, production-grade.

| Lines | Code | Why it exists |
|---|---|---|
| `/dev/ram*` | brd driver | real RAMdisk blocks (format + mount like any disk — same verbs, kernel edition) |
| `/dev/shm + tmpfs` | cousin | files in page cache (RAM speed, swap-backed spill — volatility with an asterisk) |

Change X → Y: `dd if=/dev/zero of=/dev/shm/t bs=1M count=10 2>&1 | tail -1` then `rm /dev/shm/t`. Verify: GB/s speeds (RAM, not rust — the number that justifies initramfs + test beds).

## Ship It

Artifact: `outputs/ramdisk-card.md` — verb parity table (blk vs rd, per-verb durability notes), volatility contract, `brd`/`tmpfs` verbs. Next lesson mounts a filesystem on *either* — the interface pays off immediately.

## Exercises

1. Easy — snapshot/restore helpers (`rd_snapshot`/`rd_restore` via malloc+memcpy): crash, restore, prove bytes back (persistence *layered* above volatility — the flush/to-file idea, rebuilt).
2. Medium — fault injection: `rd_flaky` wrapper failing every Nth op (counter-driven): run the demo, count graceful `-1`s (disks lie occasionally — drivers must handle, journals *do*).
3. Hard — `mmap`-backed variant (file-mapped RAM: speed + durability — the best of both; compare `time` on 10k writes vs file-backend: measure the syscall savings).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| heap | the "platters" are malloc'd (device dies with the process) | ../../glossary/terms.md#heap |
| address | block numbers address store like VPNs address RAM (numbered spaces) | ../../glossary/terms.md#address |
| syscall | none inside (pure userspace); real `brd` crosses via block layer | ../../glossary/terms.md#syscall |

## Further Reading

- Linux `drivers/block/brd.c` (skim) — request-queue RAMdisk (verbs match, locking added).
- `man 1 dd` + `man 8 mkfs` — image making for test beds (08's `mkfs` rehearsal).
- OSTEP Ch.36 re-read — RAM vs disk numbers side by side (latency table, memorized by feel now).

# Block Layer — Bytes That Survive Reboot

> RAM forgets. Disks remember — in fixed-size blocks, at glacial speed, with crashes mid-sentence.

**Type:** Build
**Languages:** C
**Prerequisites:** 03-swap-oom
**College ref:** OSTEP Ch.36–37 (disks + RAID preview), MIT 6.1810 FS labs (block cache), xv6 file kernel/bio.c + kernel/fs.h (BCACHE yours mirrors)
**Time:** ~75 minutes

## Learning Objectives
- Trace write → cache(dirty) → flush → disk vs crash-before-flush (loss) using counters
- Implement a file-backed block device: read/write/flush/crash with stats in C
- Explain writeback vs writethrough, torn writes, and why HDDs fear seeks while SSDs fear erases
- Connect the block cache to the page cache (same idea, different clientele) and to journaling (next: crash-proofing)

## Concept in 60s

![block layer](../figures/block-layer.svg)

<!-- source: ../figures/block-layer.excalidraw — open in excalidraw.com to redraw -->

Your RAM forgets when the power dies — disks remember, but only in fixed-size blocks, at glacial speed, and they can die mid-sentence. Our toy speaks 64 blocks of 64 bytes; real disks speak 512-byte to 4K sectors by the million. `write(bno)` lands in a RAM cache marked *dirty* — a fast promise — and `flush` pushes the dirty blocks to the file, the slow truth. `crash` drops whatever is still dirty, a power cut on demand: unflushed writes never happened. Spinning HDDs fear seeks measured in milliseconds, so random I/O dies; SSDs have no seeks but pay in erase-blocks and wear, so random writes cost differently. A crash mid-flush tears a write — half a block new — and checksums detect what journals repair. The stats count every read, write, flush, and drop, the counter habit from phase 04 carried forward.

## Simulate It (host C — file-backed disk, portable stdio)

Split `code/blk.h` + `code/blk.c` + `code/demo.c` (headers/split, fourth542 practice — now reflex).

```c
// blk.h -- block device shape.
#ifndef OSFS_BLK_H
#define OSFS_BLK_H

#include <stddef.h>

#define BLK_NBLOCKS 64
#define BLK_BSIZE 64

typedef struct {
    unsigned char cache[BLK_NBLOCKS][BLK_BSIZE];
    int dirty[BLK_NBLOCKS];
    const char *path;
    long reads, writes, flushes, drops;
} blkdev_t;

int blk_init(blkdev_t *d, const char *path);   // 0 ok, -1 no file
int blk_read(blkdev_t *d, int bno, unsigned char *out);
int blk_write(blkdev_t *d, int bno, const unsigned char *in); // cached+dirty
int blk_flush(blkdev_t *d);    // push dirties; returns flushed count
int blk_crash(blkdev_t *d);    // drop dirties; returns dropped count

#endif
```

What this does: publishes a 64-block toy with cache semantics — writes dirty, reads serve cache-first, flush persists, crash forgets.

```c
// blk.c -- writeback cache over a flat file.
#include <stdio.h>
#include <string.h>
#include "blk.h"

static long fsize(FILE *f) {
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    return n;
}

int blk_init(blkdev_t *d, const char *path) {
    memset(d, 0, sizeof *d);
    d->path = path;
    FILE *f = fopen(path, "r+b");
    if (!f) {
        f = fopen(path, "w+b");   // create: zero-filled below
        if (!f) return -1;
    }
    if (fsize(f) < (long)(BLK_NBLOCKS * BLK_BSIZE)) {
        unsigned char z[BLK_BSIZE] = {0};
        for (int i = 0; i < BLK_NBLOCKS; i++) fwrite(z, 1, BLK_BSIZE, f);
    }
    // warm cache from file (writeback starts clean, like boot)
    rewind(f);
    for (int i = 0; i < BLK_NBLOCKS; i++)
        if (fread(d->cache[i], 1, BLK_BSIZE, f) != BLK_BSIZE) { fclose(f); return -1; }
    fclose(f);
    return 0;
}

int blk_read(blkdev_t *d, int bno, unsigned char *out) {
    if (bno < 0 || bno >= BLK_NBLOCKS) return -1;
    memcpy(out, d->cache[bno], BLK_BSIZE);   // cache-first: dirties visible pre-flush
    d->reads++;
    return 0;
}

int blk_write(blkdev_t *d, int bno, const unsigned char *in) {
    if (bno < 0 || bno >= BLK_NBLOCKS) return -1;
    memcpy(d->cache[bno], in, BLK_BSIZE);
    d->dirty[bno] = 1;
    d->writes++;
    return 0;
}

int blk_flush(blkdev_t *d) {
    FILE *f = fopen(d->path, "r+b");
    if (!f) return -1;
    int n = 0;
    for (int i = 0; i < BLK_NBLOCKS; i++)
        if (d->dirty[i]) {
            fseek(f, (long)i * BLK_BSIZE, SEEK_SET);
            fwrite(d->cache[i], 1, BLK_BSIZE, f);
            d->dirty[i] = 0;
            n++;
        }
    fclose(f);
    d->flushes++;
    return n;
}

int blk_crash(blkdev_t *d) {
    FILE *f = fopen(d->path, "rb");   // reload = forget dirties
    if (!f) return -1;
    int n = 0;
    for (int i = 0; i < BLK_NBLOCKS; i++)
        if (d->dirty[i]) {
            fseek(f, (long)i * BLK_BSIZE, SEEK_SET);
            if (fread(d->cache[i], 1, BLK_BSIZE, f) != BLK_BSIZE) { fclose(f); return -1; }
            d->dirty[i] = 0;
            n++;
        }
    fclose(f);
    d->drops++;
    return n;
}
```

What this does: writeback caching with honest crash semantics — flush persists, crash reloads pre-dirt (the power-cut time machine).

| Lines | Code | Why it exists |
|---|---|---|
| 20–23 | create-if-absent | `r+b` fails fresh → `w+b` creates (first boot formats — filesystems do exactly this once) |
| 24–27 | zero-extend | short file grows to full geometry (sparse file on Linux stays sparse — size lies, blocks allocate on write, like mmap) |
| 29–32 | warm cache | boot loads disk into cache *clean* (buffer cache starts truthful; dirt accumulates only via writes) |
| 38 | cache-first read | unflushed writes visible immediately (coherence: readers see latest, flush decides durability — two different promises!) |
| 53–63 | flush loop | only dirties seek+write (clean blocks skip I/O — the economy writeback exists for) |
| 66–80 | crash = reload | dirties re-read from file (forget the lie, restore truth; `drops` counts the amnesia for stats) |

Change X → Y: skip `blk_flush` before `blk_crash` in the demo (already the demo's first act — read why it must print OLD data: durability requires the flush call, not the write call).

```c
// demo.c -- crash loses unflushed; flush survives crash.
#include <stdio.h>
#include <string.h>
#include "blk.h"

static void fill(unsigned char *b, char c) { memset(b, c, BLK_BSIZE); }

int main(void) {
    blkdev_t d;
    remove("build/disk.img");   // fresh geometry each run (deterministic!)
    if (blk_init(&d, "build/disk.img") != 0) return 1;
    unsigned char b[BLK_BSIZE], out[BLK_BSIZE];
    fill(b, 'A');
    blk_write(&d, 3, b);        // dirty, unflushed
    printf("dropped=%d ", blk_crash(&d));
    blk_read(&d, 3, out);
    printf("after-crash-1st-byte=%d (0=zeroed, 65=A)\n", out[0]);
    fill(b, 'B');
    blk_write(&d, 5, b);
    printf("flushed=%d ", blk_flush(&d));
    printf("dropped=%d ", blk_crash(&d));
    blk_read(&d, 5, out);
    printf("after-flush-1st-byte=%d\n", out[0]);
    printf("reads=%ld writes=%ld flushes=%ld drops=%ld\n",
           d.reads, d.writes, d.flushes, d.drops);
    return out[0] != 'B';
}
```

What this does: proves the two promises separately — crash eats unflushed `A` (byte 0), flush saves `B` through a later crash (durability = flush, not write).

| Lines | Code | Why it exists |
|---|---|---|
| 10 | `remove` first | deterministic geometry (stale `disk.img` would poison reruns — tests need clean rooms) |
| 13–16 | write+crash+read | `A` never flushed → byte reads 0 (the loss, numeric: 65 expected, 0 found) |
| 18–22 | write+flush+crash+read | `B` flushed → survives (byte 66 — durability demonstrated, not asserted) |

Change X → Y: flush *before* the first crash too. Verify: first byte prints 65 (the control experiment — same crash, different history; flush is the variable).

## Build It

```bash
make run
make test
```

What this does: runs the loss/survival diptych, then asserts roundtrip, flush-persistence (reopen!), crash-loss, OOB bounds.

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | expect `after-crash-1st-byte=0`, `after-flush-1st-byte=66`, stats line |
| `make test` | machine proof | re-`blk_init` on the flushed file (persistence across *opens* — the reboot rehearsal) |

Change X → Y: delete `remove(...)`, run twice. Verify: second run's first-crash byte is 66 (yesterday's flush = today's truth — persistence *across runs*, the point of disks).

## Use It (Linux)

Real block truth under your feet:

```bash
lsblk -d -o NAME,SIZE,ROTA,TYPE | head -6
cat /sys/block/$(lsblk -ndo NAME / | head -1)/queue/rotational 2>/dev/null || echo "(container: no block sysfs)"
df -h . | tail -1
```

What this does: lists disks with rotation flags (1=HDD seeks hurt, 0=SSD/flash), then your filesystem's free space (the budget crashes spend).

| Lines | Code | Why it exists |
|---|---|---|
| `ROTA` | seek truth | 1 = schedule for seeks (elevators!), 0 = schedule for erase/wear (different physics, different scheduler) |
| `df` | budget | full disks fail writes mid-flush (torn-write's mundane cousin — space exhaustion) |

Change X → Y: `cat /sys/block/*/queue/scheduler 2>/dev/null | head -3`. Verify: `[mq-deadline]`/`none` flavors (I/O schedulers pick *order* — the disk-side cousins of our CPU schedulers, same theory, slower medium).

## Ship It

Artifact: `outputs/blk-card.md` — writeback-vs-writethrough rule, flush/crash verbs, torn-write note, `lsblk`/`df` readers. Persistence reference page one (VSFS builds blocks into files next). You now own durability's ground floor — the RAMdisk next door replays these same verbs with the platters removed.

## Exercises

1. Easy — write all 64 blocks with distinct first-bytes, flush, `xxd build/disk.img | head -4` (your bytes on "platters" — file as disk, visible).
2. Medium — count syscalls: `strace -c -e trace=read,write,lseek` one flush of 3 dirties (seeks+lseek per block — batching argument for extents, previewed numerically).
3. Hard — torn-write detector: checksum per block (first 4 bytes = rest's sum), crash *mid-flush* simulated by flushing only even dirties then crashing (odd dirties lost, evens kept — partial durability observed; journals exist to forbid exactly this state).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| syscall | `read`/`write`/`fsync` under every block op (durability bottoms at `fsync`) | [syscall](../../../../glossary/terms.md#syscall) |
| heap | cache lives in heap RAM (dirty bitmap = the truth ledger) | [heap](../../../../glossary/terms.md#heap) |
| address | block numbers address disk the way VPNs address RAM (numbers spaces) | [address](../../../../glossary/terms.md#address) |

## Further Reading

- OSTEP Ch.36–37 — HDD mechanics + RAID (the physics + redundancy chapters).
- xv6 `kernel/bio.c` — BCACHE LRU + sleep locks (our cache, grown locking + eviction).
- `man 2 fsync,fdatasync` — durability's real verbs (flush-to-OS vs flush-to-platter).

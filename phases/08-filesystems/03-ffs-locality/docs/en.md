# FFS Intuition — Keep Friends Close (Locality Wins Disks)

> First-fit scatters. Clustered gathers. The disk head thanks only one of them.

**Type:** Learn
**Languages:** C
**Prerequisites:** 02-journaling
**College ref:** OSTEP Ch.40–41 (FFS + LFS preview), MIT 6.1810 FS layout notes, xv6 `kernel/fs.c` (direct blocks yours extends)
**Time:** ~60 minutes

## Learning Objectives
- Trace 8-block allocation under first-fit vs clustered-from-inode over one fragmented bitmap
- Implement both policies + span/gap metrics over a shared bitmap in C
- Explain cylinder/block groups (inode near data, dirs near inodes) and why contiguity halves seeks
- Connect placement to the allocator zoo (06: bump clusters naturally, lists scatter) and to SSDs (locality still wins caches)

## Concept in 60s

![locality groups](../figures/locality-groups.svg)

<!-- source: ../figures/locality-groups.excalidraw — open in excalidraw.com to redraw -->

Old files die leaving holes: evens 0–30 occupied, odds free (Swiss cheese). A new file (inode at block 40) needs 8 blocks. **First-fit** scans from 0: takes odds 1–15 (span 14, gaps 14 — seeks everywhere). **Clustered** scans up from the inode: takes 41–48 contiguous (span 7, gaps 7 — one short glide). FFS generalizes: split disk into cylinder/block *groups*, each with own inodes+bitmap+data; place file data in its inode's group, dirs near parents, big files across groups. Same bytes, half the seeks. SSDs don't seek — but caches/prefetch still love contiguity (locality never retires, only changes currency).

## Simulate It (host C — two policies, one bitmap, no QEMU)

Core: `code/ffs.h` + `code/ffs.c` (bitmap + both policies + metrics). Driver: `code/main.c` (fragment identically, allocate both ways).

```c
// main.c -- same holes, two policies.
#include <stdio.h>
#include "ffs.h"

static void fragment(ffs_t *f) {
    for (int b = 0; b <= 30; b += 2) ffs_mark(f, b);  // evens 0..30: old holes
    ffs_mark(f, 40);                                   // the new file's inode block
}

int main(void) {
    ffs_t a, b;
    int fa[8], cl[8];
    ffs_init(&a);
    fragment(&a);
    ffs_first_fit(&a, 8, fa);
    ffs_init(&b);
    fragment(&b);
    ffs_clustered(&b, 40, 8, cl);
    printf("firstfit: span=%d gaps=%d\n", ffs_span(fa, 8), ffs_gaps(fa, 8));
    printf("clustered: span=%d gaps=%d\n", ffs_span(cl, 8), ffs_gaps(cl, 8));
    return 0;
}
```

What this does: builds identical Swiss cheese twice, allocates 8 blocks each way, prints span (max−min) + gaps (seek-distance sum) — placement policy as two numbers.

| Lines | Code | Why it exists |
|---|---|---|
| 5–8 | `fragment` | evens 0–30 + inode 40 occupied (17 used): holes below, fresh group above — the OldFS-vs-FFS courtroom, staged |
| 13–14 | first-fit run | scans 0→63: takes odds 1–15 (lowest free — blind to the inode at 40) |
| 16–17 | clustered run | scans 40→63: takes 41–48 contiguous (inode-near — FFS's one-line religion) |
| 18–19 | metrics print | span 14-vs-7, gaps 14-vs-7 (2× win, counted not claimed) |

Change X → Y: move the inode mark `40` → `4` (inode amid the holes). Verify: clustered takes nearby odds too (policy degrades gracefully — near-hole inode, hole-ish data; groups help most when fresh space exists).

```c
int ffs_clustered(ffs_t *f, int near, int n, int *out) {
    int got = 0;
    for (int d = 0; d < F_NBLOCKS && got < n; d++) {
        int b = near + d;
        if (b >= 0 && b < F_NBLOCKS && !f->used[b]) {
            f->used[b] = 1;
            out[got++] = b;
        }
    }
    return got == n ? n : -1;
}
```

What this does: upward scan from `near`, taking free blocks in order — contiguity preferred, scatter tolerated (degrades, never fails while space exists).

| Lines | Code | Why it exists |
|---|---|---|
| `near + d` upward | affinity direction | groups extend forward (real FFS: same group first, then neighbors — direction barely matters, affinity does) |
| `-1` on shortfall | honest exhaustion | ENOSPC as a return code (the error VSFS create paths must propagate — 08/01's `-1`s grow teeth here) |

Change X → Y: scan *both* directions (nearest-first: 40,39,41,38...). Verify on this bitmap: ping-pong order (39 used? 39 free... takes 41? compute: d0→40 used, d1→39 free? nearest-first listed 40,41,39,42,38... takes 41,39,42,38,43,37,44,36: span 8, gaps huge zigzag — nearest ≠ clustered! Order matters: upward-sweep beats nearest-jitter for seeks).

## Build It

```bash
make run
make test
```

What this does: prints `span=14 gaps=14` vs `span=7 gaps=7`, then asserts both policies' sets, metrics, OOB guards, and exhaustion honesty.

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | the 2×, byte-exact (14/14 vs 7/7 — the lesson's thesis in four numbers) |
| `make test` | machine proof | same engine, asserted (placement is policy: pin the sets, not just the scores) |

Change X → Y: request 60 blocks (nearly all). Verify: both policies converge (no room to choose — scarcity erases policy; plenty reveals it; the capacity moral).

## Use It (Linux)

Real filesystems expose grouping:

```bash
df -h . | tail -1
dumpe2fs -h $(df / --output=source 2>/dev/null | tail -1) 2>/dev/null | grep -iE "block size|blocks per group|inodes per group" | head -4 || echo "(need extN rights; debugfs alternative below)"
debugfs -R "stat <2>" $(df / --output=source 2>/dev/null | tail -1) 2>/dev/null | head -8 || echo "(same rights note; toy above stands alone)"
```

What this does: shows block/group geometry knobs (blocks-per-group = FFS's group size, production edition) and root inode's block pointers (direct extents near group — our direct[4] at datacenter scale).

| Lines | Code | Why it exists |
|---|---|---|
| `blocks per group` | group size | ~32k blocks/group ext4 (our 64-block toy, grown 500× — same religion, bigger parishes) |
| `stat <2>` | root's blocks | extent list clustered (read the block numbers: neighbors, not confetti — FFS working, observed) |

Change X → Y: `filefrag code/demo.c 2>/dev/null || echo no-filefrag` (Linux: extent count per file). Verify: few extents (1–3 typical — allocator clustered at birth; aged disks fragment, `e4defrag` exists for the aftermath).

## Ship It

Artifact: `outputs/locality-card.md` — first-fit vs clustered rule, span/gap metrics, group recipe (inode-near-data, dirs-near-parents, big-files-across), SSD translation (seeks→cache/prefetch). Filesystem reference complete (08: VSFS → journal → FFS).

## Exercises

1. Easy — allocate 3 files × 8 blocks interleaved (A,B,C,A,B,C... chunk by chunk): show all three scatter under both policies (interleave defeats affinity — preallocation `fallocate` exists for exactly this).
2. Medium — implement `ffs_reserve(inode_block)` (hold nearby run at create time): measure 8-block file span with/without reservation (reservation = affinity with teeth).
3. Hard — LFS sketch: always append data+inode to log end (no in-place update ever): compare write cost (sequential, glorious) vs read cost (scattered, needs index) vs crash recovery (replay from checkpoint — journal thinking, filesystem scale; the Ch.43–44 bridge).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| inode | lives in groups too (its block anchors the affinity search) | [inode](../../../../glossary/terms.md#inode) |
| address | block numbers cluster by policy (placement is addressing with taste) | [address](../../../../glossary/terms.md#address) |
| heap | bitmaps are heap/cached RAM (placement scans must be fast — 64 bits here, 32k bits there) | [heap](../../../../glossary/terms.md#heap) |

## Further Reading

- OSTEP Ch.40–41 — FFS + LFS (this lesson runs 40's experiment; 41 inverts it gloriously).
- `man 8 mkfs.ext4` (`-g blocks-per-group`, `-i bytes-per-inode` — group knobs, user-settable).
- `man 1 filefrag` + `e4defrag` — fragmentation observed + repaired (the aftermath tools).

# VSFS — Files on Blocks: Super, Bitmap, Inodes, Dirs

> Names live in directories. Metadata lives in inodes. Bits live in blocks. The bitmap knows which.

**Type:** Build
**Languages:** C
**Prerequisites:** 02-ramdisk
**College ref:** OSTEP Ch.39–40 (VSFS + FFS preview), MIT 6.1810 FS labs (inode/dir layout), xv6 file kernel/fs.h + kernel/fs.c (dinode/dirent yours mirrors)
**Time:** ~90 minutes

## Learning Objectives
- Trace `create → write → read` through superblock, bitmap, inode, dir-entry, and data blocks
- Implement mkfs/create/write/read/list over backend function pointers (RAM here, any disk later)
- Explain direct blocks, sizes, and why 4×64B caps files at 256B (and how indirect fixes it)
- Connect bitmap allocation to bump/buddy thinking (free-space structures, reused) and dirs to names→numbers maps

## Concept in 60s

![vsfs layout](../figures/vsfs-layout.svg)

<!-- source: ../figures/vsfs-layout.excalidraw — open in excalidraw.com to redraw -->

64 blocks × 64B: block 0 = superblock (magic + geometry: nblocks, ninodes, inode/data starts), block 1 = bitmap (1 bit per block: 8 bytes cover 64), blocks 2–5 = 8 inodes × 32B (type/size/direct[4]/pad), blocks 6+ = data; root dir = inode 0, its data holds 16B entries (12B name + u32 inum, 4 per block). `create("hi")`: find free inode (scan types), append dir entry to root, done (empty file, size 0). `write(inum, data, len)`: allocate `ceil(len/64)` bitmap blocks, stamp `direct[]`, set size. `read`: walk direct blocks to `size` (never past — short final block handled by `size`, not block count). Backend = two function pointers (RAM array here — 07/02's verbs wearing a vtable; file/disk backends plug unchanged).

## Simulate It (host C — RAM backend, portable)

Backend shim shared by demo+tests (`code/rambackend.h` — 10 lines, no duplication):

```c
// rambackend.h -- RAM blocks behind vsfs ops. Lesson docs/en.md.
#ifndef OSFS_RAMBACKEND_H
#define OSFS_RAMBACKEND_H

#include <string.h>
#include "vsfs.h"

typedef struct { unsigned char d[VSB_NBLOCKS][VSB_BSIZE]; } ramdisk_mem_t;

static int ram_r(void *ctx, int bno, unsigned char *out) {
    if (bno < 0 || bno >= VSB_NBLOCKS) return -1;
    memcpy(out, ((ramdisk_mem_t *)ctx)->d[bno], VSB_BSIZE);
    return 0;
}

static int ram_w(void *ctx, int bno, const unsigned char *in) {
    if (bno < 0 || bno >= VSB_NBLOCKS) return -1;
    memcpy(((ramdisk_mem_t *)ctx)->d[bno], in, VSB_BSIZE);
    return 0;
}

#endif
```

What this does: turns a static 2D array into a block device the FS can't distinguish from a disk (backend-blindness, proven by construction).

```c
// vsfs.h -- geometry + ops + verbs.
#ifndef OSFS_VSFS_H
#define OSFS_VSFS_H

#include <stddef.h>

#define VSB_NBLOCKS 64
#define VSB_BSIZE 64
#define VSB_NINODES 8
#define VSB_NDIRECT 4
#define VSB_MAXFILE (VSB_NDIRECT * VSB_BSIZE)  // 256: direct-only ceiling
#define VSB_NAMELEN 12

typedef struct {
    int (*read)(void *ctx, int bno, unsigned char *out);
    int (*write)(void *ctx, int bno, const unsigned char *in);
    void *ctx;
} vsfs_ops_t;

int vsfs_mkfs(vsfs_ops_t *ops);
int vsfs_create(vsfs_ops_t *ops, const char *name);          // inum or -1
int vsfs_write(vsfs_ops_t *ops, int inum, const unsigned char *data, size_t len);
int vsfs_read(vsfs_ops_t *ops, int inum, unsigned char *out, size_t cap); // bytes or -1

#endif
```

What this does: fixes geometry (super/bitmap/inodes/data map below) and four verbs — create names, write fills, read returns `size`-bounded bytes.

| Lines | Code | Why it exists |
|---|---|---|
| 7–12 | geometry defines | single source of layout truth (super=0, bitmap=1, inodes=2–5, data=6+; change here, everything follows — magic numbers nowhere else) |
| 12 | `VSB_MAXFILE` 256 | direct-only ceiling stated upfront (4 blocks × 64B — indirect pointers lift it in FFS lesson, the limitation named before felt) |
| 14–18 | ops vtable | two function pointers + ctx (filesystems program to verbs — 07/02's payoff: RAM/file/disk plug here) |

```c
// vsfs.c -- super, bitmap, inodes, root dir. (Abridged here; full file in code/.)
#include <string.h>
#include "vsfs.h"

#define SB_MAGIC 0x76667331u /* "vfs1" */
#define INODE_BLOCK0 2
#define DATA_BLOCK0 6
#define T_FILE 1
#define T_DIR 2

typedef struct { unsigned type; unsigned size; unsigned direct[VSB_NDIRECT]; unsigned pad[2]; } inode_t; // 32B
typedef struct { char name[VSB_NAMELEN]; unsigned inum; } dirent_t; // 16B

static int blk_rd(vsfs_ops_t *o, int b, unsigned char *out) { return o->read(o->ctx, b, out); }
static int blk_wr(vsfs_ops_t *o, int b, const unsigned char *in) { return o->write(o->ctx, b, in); }
```

What this does (with the elided helpers): bitmap scans for free blocks, inode read/write packs 2-per-block, dir ops scan root entries — each helper follows in `code/vsfs.c` with the same table below.

| Part | Rule |
|---|---|
| superblock | block 0: magic + counts (mkfs stamps; `create` asserts magic first — mounting verifies) |
| bitmap | block 1 bytes: bit=block (blocks 0–5 pre-marked used at mkfs — metadata protects itself) |
| inodes | 32B each, 2 per block, blocks 2–5 (offset math: `block = 2 + inum/2`, `off = (inum%2)*32` — P-02/03 addressing, filesystem edition) |
| root dir | inode 0, type dir, entries 16B in its data blocks (name→inum map; lookup scans — `ls` speed is O(files), FFS fixes with better structures later) |

`code/demo.c` flow (full file in repo): mkfs → create "hello" → write message → read back → list root → print usage. Expected:

```text
mkfs: ok
create hello -> inum 1
read back: hello vsfs! (size 11)
root: hello(1)
blocks used: 8/64
```

What this does: the filesystem's first day — format, name, fill, verify, census (8 used = super+bitmap+4 inode blocks+root data+1 file block... counted in code, asserted in tests).

Change X → Y: write 300 bytes (over `VSB_MAXFILE` 256). Verify: `-1` (direct ceiling enforced — the limitation stated in the header, demonstrated; indirect pointers are 08's next room).

## Build It

```bash
make run
make test
```

What this does: runs the first-day flow, then asserts roundtrip, sizes, duplicate-create refusal, missing-file -1, over-max refusal, bitmap accounting.

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | the five lines above, byte-exact (names, sizes, census) |
| `make test` | machine proof | real `vsfs.c` linked (not copies): create/write/read/list/duplicate/missing/max/bitmap |

Change X → Y: create `"hello"` twice. Verify: second `-1` (names unique in root — dir scan rejects dupes; the namespace invariant, enforced).

## Use It (Linux)

Real supers/blocks/inodes under your feet:

```bash
df -i . | tail -1
dumpe2fs -h /dev/null 2>&1 | head -2 || echo "(need a real extN dev; try: tune2fs -l \$(df / --output=source | tail -1) 2>/dev/null | head -8)"
stat build/../demo 2>/dev/null || stat code/demo.c
```

What this does: shows inode consumption (`IUse%` — inodes exhaust *independently* of blocks!), superblock dumps where permitted, and a file's size/blocks/inode (your `stat` = our inode fields, production-flavored).

| Lines | Code | Why it exists |
|---|---|---|
| `df -i` | inode budget | `IFree` hitting 0 breaks creates with space free (the bitmap lesson's twin: two scarcities, tracked separately) |
| `stat` | inode view | `Size/Blocks/IO Block/Inode` = size/direct-count/BLOCKSIZE/inum (our struct, ext-sized) |

Change X → Y: `df -i /tmp` vs `df -i /` (if tmpfs present). Verify: different totals (each mount = own super+bitmaps — filesystems are per-device universes, like address spaces per process).

## Ship It

Artifact: `outputs/vsfs-card.md` — layout map (0/1/2–5/6+), inode/dir entry structs, verb contracts, ceiling note (256B direct-only), bitmap verbs. 08/02 journals *these exact blocks* (the card becomes the journal's vocabulary).

## Exercises

1. Easy — fill root (16 files? no — 8 dir slots... count actual max: root data blocks allocated on demand: create until -1, report N (namespace capacity, measured).
2. Medium — `vsfs_delete(name)` (free data bitmap bits + clear inode + remove dirent): create/write/delete/recreate cycle with bitmap counts proving reclamation (delete is allocation in reverse — bitmaps prove it).
3. Hard — indirect block: 5th `direct` slot becomes single-indirect (16 block numbers × 64B = +1 KiB/file): rewrite MAXFILE math, extend write/read, test a 300B file (the ceiling lifts by pointer-chasing — FFS's core trick, earned).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| inode | number → (type/size/blocks) card; names live in dirs, not here | [inode](../../../../glossary/terms.md#inode) |
| address | block numbers address disk (super/bitmap/inode/data regions by convention) | [address](../../../../glossary/terms.md#address) |
| syscall | `open/read/write/stat` resolve through exactly these structures (dents+inodes) | [syscall](../../../../glossary/terms.md#syscall) |
| heap | bitmaps/inode bufs live caller-side (FS code is allocation-disciplined, like 06) | [heap](../../../../glossary/terms.md#heap) |

## Further Reading

- OSTEP Ch.39–40 — VSFS + FFS (this lesson runs Ch.39's examples; Ch.40 motivates Exercise 3).
- xv6 `kernel/fs.h` (`dinode`/`dirent`) + `kernel/fs.c` (`ialloc/balloc` — bitmap twins of ours).
- `man 2 stat` — the inode fields userspace may see.

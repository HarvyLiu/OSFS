# Multi-Level Page Tables in Code — Directories All the Way Down

> One giant table wastes megabytes per process. Two small levels cost one extra lookup — and empty branches cost nothing.

**Type:** Build
**Languages:** C
**Prerequisites:** 02-segmentation-paging
**College ref:** OSTEP Ch.20 (multi-level tables), MIT 6.1810 Lec 8 (Sv39 walk), xv6 file kernel/vm.c (`walk`/`mappages` yours mirrors, 2 levels not 3)
**Time:** ~90 minutes

## Learning Objectives
- Trace a 32-bit VA through dir (10b) → table (10b) → offset (12b) with a worked example
- Implement on-demand table allocation, map/translate/unmap with P/RW/U flags in C
- Explain why empty address ranges cost zero bytes (NULL dir slots) and what a walk costs (2 lookups)
- Connect the root pointer to the PCB field + CR3 (one register names the whole space)

## Concept in 60s

![multilevel walk](../figures/multilevel-walk.svg)

<!-- source: ../figures/multilevel-walk.excalidraw — open in excalidraw.com to redraw -->

A flat 32-bit table needs 2²⁰ entries × 4B = 4 MiB *per process* (sparse spaces mostly pay for air). Split the 20-bit VPN into dir(10b)+table(10b): 1024-entry directory, each live slot points at a 1024-entry page table (4 KiB exactly — one page per table, how convenient), each PTE holds frame+flags. Empty 4 MiB region = one NULL dir slot (4 bytes, not 1024×4). Walk: `pd = va>>22`, `pt = va>>12&0x3FF`, `off = va&0xFFF`: dir → table → frame → `frame*4096+off`. PTE bits: P(esent), RW, U(ser) — the enforcement trio. Root = the [PCB](../../../../glossary/terms.md#pcb)'s page-table pointer = CR3's value (one [register](../../../../glossary/terms.md#register) names a universe).

## Simulate It (host C — the walk, no QEMU)

Split `code/pt.h` + `code/pt.c` + `code/main.c` (tables deserve modules).

```c
// pt.h -- 2-level 32-bit tables: 10/10/12.
#ifndef OSFS_PT_H
#define OSFS_PT_H

#include <stdint.h>

#define PT_ENTRIES 1024
#define PTE_P 0x1u
#define PTE_RW 0x2u
#define PTE_US 0x4u

typedef struct { uint32_t e[PT_ENTRIES]; } ptable_t;  // 4 KiB exactly
typedef struct { ptable_t *dir[PT_ENTRIES]; } pdir_t; // NULL = empty 4M

pdir_t *pt_new(void);
void pt_free(pdir_t *root);
int pt_map(pdir_t *root, uint32_t va, uint32_t pa, uint32_t flags);
int pt_unmap(pdir_t *root, uint32_t va);
// is_write=1 enforces RW. 0 ok; -2 no table; -3 no page; -4 ro-write.
int pt_translate(pdir_t *root, uint32_t va, int is_write, uint32_t *pa);

#endif
```

What this does: publishes the two-level shape — directory of table-pointers, flags, and four verbs with numbered faults (05/02's triage habit, leveled up).

| Lines | Code | Why it exists |
|---|---|---|
| 11 | `ptable_t` 1024×u32 | exactly 4096 bytes = one page per table (allocator-friendly: tables *are* pages in real kernels) |
| 12 | `pdir_t` of pointers | NULL slot = empty 4 MiB for 8 bytes... 4 bytes (the sparsity win: absence is cheap) |
| 14–17 | flags P/RW/US | present (mapped?), writable (else read-only), user (else kernel-only — Protection's favorite bit) |
| 20 | numbered faults | -2 dir-miss, -3 page-miss, -4 ro-write: *which* level failed tells you *what* to fix (map wide vs map page vs fix perms) |

```c
// pt.c -- walk with on-demand tables.
#include <stdlib.h>
#include "pt.h"

pdir_t *pt_new(void) {
    pdir_t *r = calloc(1, sizeof *r);
    return r; // NULL on OOM: caller checks (P-01/04 discipline, tables too)
}

void pt_free(pdir_t *root) {
    if (!root) return;
    for (int i = 0; i < PT_ENTRIES; i++) free(root->dir[i]);
    free(root);
}

static ptable_t *get_table(pdir_t *root, uint32_t va, int create) {
    uint32_t pd = (va >> 22) & 0x3FF;
    if (!root->dir[pd]) {
        if (!create) return 0;
        root->dir[pd] = calloc(1, sizeof(ptable_t));
        if (!root->dir[pd]) return 0;
    }
    return root->dir[pd];
}

int pt_map(pdir_t *root, uint32_t va, uint32_t pa, uint32_t flags) {
    ptable_t *t = get_table(root, va, 1);
    if (!t) return -1;
    t->e[(va >> 12) & 0x3FF] = (pa & 0xFFFFF000u) | (flags & 0xFFFu) | PTE_P;
    return 0;
}

int pt_unmap(pdir_t *root, uint32_t va) {
    ptable_t *t = get_table(root, va, 0);
    if (!t) return -2;
    t->e[(va >> 12) & 0x3FF] = 0;
    return 0;
}

int pt_translate(pdir_t *root, uint32_t va, int is_write, uint32_t *pa) {
    ptable_t *t = get_table(root, va, 0);
    if (!t) return -2;
    uint32_t e = t->e[(va >> 12) & 0x3FF];
    if (!(e & PTE_P)) return -3;
    if (is_write && !(e & PTE_RW)) return -4;
    *pa = (e & 0xFFFFF000u) | (va & 0xFFFu);
    return 0;
}
```

What this does: allocates tables lazily, stamps frames+flags, walks with enforcement — `walk`+`mappages` from xv6 in miniature.

| Lines | Code | Why it exists |
|---|---|---|
| 4–7 | `calloc` root | zeroed = all NULL = empty universe for ~8 KiB (vs 4 MiB flat — the win, quantified at birth) |
| 15–24 | `get_table` | dir index → table or NULL; `create` splits map-path (allocate) from walk-path (never allocate on lookup — faults map, lookups don't) |
| 28–33 | map masks | `pa & ...F000` keeps frame, `flags & 0xFFF` keeps low bits, `\| PTE_P` always (mapping *means* present — call `unmap` to take back) |
| 42–50 | translate enforces | dir-miss → page-miss → perm-check → combine (order matters: cheap structural faults before semantic ones) |

```c
// main.c -- three mappings, four walks.
#include <stdio.h>
#include "pt.h"

int main(void) {
    pdir_t *root = pt_new();
    if (!root) return 1;
    pt_map(root, 0x00401000, 0x00007000, PTE_RW | PTE_US);
    pt_map(root, 0x00402000, 0x00008000, PTE_US);          // read-only
    pt_map(root, 0xC0000000, 0x00100000, PTE_RW);          // kernel-ish, no US
    uint32_t pa = 0;
    // NOTE: call-then-print (C printf arg order is unspecified; calling inside
    // prints stale pa -- the same bug class 05/02 documents).
    int r1 = pt_translate(root, 0x00401ABC, 0, &pa);
    printf("read 0x00401ABC -> %s 0x%x\n", r1 == 0 ? "ok" : "FAULT", pa);
    int r2 = pt_translate(root, 0x00402ABC, 1, &pa);
    printf("write 0x00402ABC -> %s (RO page)\n", r2 == 0 ? "ok" : "FAULT");
    int r3 = pt_translate(root, 0x00800000, 0, &pa);
    printf("read 0x00800000 -> %s (never mapped)\n", r3 == 0 ? "ok" : "FAULT");
    int r4 = pt_translate(root, 0xC0000ABC, 0, &pa);
    printf("read 0xC0000ABC -> %s 0x%x (high half)\n", r4 == 0 ? "ok" : "FAULT", pa);
    pt_free(root);
    return !(r1 == 0 && r2 == -4 && r3 != 0 && r4 == 0);
}
```

What this does: maps user-RW, user-RO, and supervisor pages, then walks reads, a forbidden write, a hole, and the high half — the permission matrix in four lines of output.

| Lines | Code | Why it exists |
|---|---|---|
| 8–10 | three maps | RW-user (normal), RO-user (code pages wear this), RW-supervisor (kernel data: no US = userspace faults — Protection's wall, previewed) |
| 13–20 | four walks | ok-read / RO-write-fault(-4) / hole-fault / high-half-ok: every PTE bit earns its line |
| 21–22 | free + gate | `pt_free` (hygiene — tables are heap) and exit code asserting exact fault numbers (not just ok/FAULT) |

Change X → Y: map `0x00402000` with `PTE_RW` added. Verify: write walk flips FAULT→ok (proves the -4 came from the bit, not the address — flags are data).

## Build It

```bash
make run
make test
```

What this does: prints the four walks, then asserts translations, all fault codes, unmap-then-fault, and remap-overwrite.

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | expect `ok 0x7abc`, `FAULT (RO)`, `FAULT (hole)`, `ok 0x100abc` |
| `make test` | machine proof | same engine, exact numbers (offset `0xABC` survives two levels — passthrough holds at depth) |

Change X → Y: `pt_unmap` the first page, re-translate. Verify: -3 (unmap clears P — presence is a bit, absence is the default; allocation and mapping are separate facts).

## Use It (Linux)

Count your own tables' cost:

```bash
./build/ptdemo
cat /proc/self/status | grep -E "VmSize|VmRSS|RssAnon"
```

What this does: runs the toy (3 pages mapped, 1 table allocated), then shows a real process's virtual vs resident sizes — the gap is sparsity + laziness at production scale.

| Lines | Code | Why it exists |
|---|---|---|
| `VmSize` vs `VmRSS` | promise vs residence | Size = mapped universe (page tables cover it), RSS = touched frames (faults delivered) — 05/01's promise/delivery split, measured |

Change X → Y: compare `VmSize` before/after `mmap`-ing 1 GiB in a scratch program (Exercise from 05/01). Verify: Size +1G, RSS ~unchanged (tables map promises cheaply — multi-level sparsity at gigascale).

## Ship It

Artifact: `outputs/walk-card.md` — 10/10/12 split, walk pseudocode (dir→table→frame), flag trio, fault-number triage (-2/-3/-4), root-per-PCB rule. The Memory reference card: 05/06's allocator and 10's `vm.c` both assume it.

## Exercises

1. Easy — translate `0x00401FFF` and `0xC0000FFF` by hand (max offsets — boundary arithmetic), confirm via `make run` additions.
2. Medium — add a U-bit check (`is_user` param, supervisor page + usermode read → -5). Wire tests (Protection's wall, now load-bearing in your walker).
3. Hard — count bytes: instrument `get_table` allocations, map 1 page vs 1024 scattered pages, report table overhead each (sparsity quantified: 4 KiB table per 4 MiB region touched — the economy that funds 64-bit spaces).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| page | 4 KiB frame unit; tables map VPNs to frames, offsets ride free | [page](../../../../glossary/terms.md#page) |
| TLB | caches walk results (this walk runs on miss — 06 Memory II prices it) | [tlb](../../../../glossary/terms.md#tlb) |
| PCB | carries the root (per-process universe in one pointer — CR3's value) | [pcb](../../../../glossary/terms.md#pcb) |
| address | dir/table/offset fields *inside* the number (never translated whole) | [address](../../../../glossary/terms.md#address) |

## Further Reading

- OSTEP Ch.20 — multi-level tables + effective-access-time math (walk cost quantified).
- xv6 `kernel/vm.c:walk/mappages` — the 3-level originals (yours, one level fewer).
- Intel SDM Vol.3 Ch.4 Fig.4-8 — the 4-level walk (same recursion, one deeper).

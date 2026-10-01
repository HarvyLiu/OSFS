# TLB — The Cache That Makes Paging Affordable

> Every access would walk tables. The TLB remembers yesterday's walks. Locality pays; strides pay double.

**Type:** Learn
**Languages:** C
**Prerequisites:** 01-bump-allocator
**College ref:** OSTEP Ch.19 (TLBs + effective access time), MIT 6.1810 Lec 8 (TLB + ASIDs), Intel SDM Vol.3 Ch.4.10 (caching translation)
**Time:** ~60 minutes

## Learning Objectives
- Trace VPN lookups through a 4-entry FIFO TLB over a fixed trace, counting hits/misses by hand
- Implement TLB + flat-table walker with EAT math (hit=101, miss=201 cycles in our model)
- Explain flush-on-switch (CR3 reload), ASIDs (flush avoided), and hugepages (fewer entries, more reach)
- Connect TLB pressure to page size choice and to scan-vs-loop workload design

## Concept in 60s

![tlb lookup](../figures/tlb-lookup.svg)

<!-- source: ../figures/tlb-lookup.excalidraw — open in excalidraw.com to redraw -->

Without cache, each access pays walk+data (our model: 1 TLB probe + 100 walk + 100 data = 201 on miss). With a hit, the cached frame skips the walk (1 + 100 = 101). Four-entry FIFO TLB over trace `0,1,2,3,0,1,4,0`: fills on 0–3 (4 misses), hits 0,1 (2 hits), 4 evicts 0 (miss), 0 misses again (evicted!) → 2 hits / 6 misses, EAT = (2×101 + 6×201)/8 = 176. Locality is the whole game: repeats hit, strides miss. Real hardware: ~64–1536 entries, LRU-ish, per-CPU (shootdowns on unmap!), flushed on CR3 switch unless ASID-tagged (PCIDs let entries survive switches).

## Simulate It (host C — TLB + walker, no QEMU)

Full program: `code/tlb.c`. FIFO cache in front of the flat table from 05/02.

```c
#include <stdio.h>

#define NPT 16
#define NTLB 4
#define T_HIT 101
#define T_MISS 201

static int ptab[NPT];

typedef struct { int vpn; int frame; int valid; } tlb_e;
static tlb_e tlb[NTLB];
static int tlb_next = 0; // FIFO hand
static long hits = 0, misses = 0;

static int tlb_lookup(int vpn, int *frame) {
    for (int i = 0; i < NTLB; i++)
        if (tlb[i].valid && tlb[i].vpn == vpn) {
            *frame = tlb[i].frame;
            hits++;
            return 1;
        }
    return 0;
}

static void tlb_insert(int vpn, int frame) {
    tlb[tlb_next].vpn = vpn;
    tlb[tlb_next].frame = frame;
    tlb[tlb_next].valid = 1;
    tlb_next = (tlb_next + 1) % NTLB;
    misses++;
}

static int access(int vpn) {
    int frame;
    if (tlb_lookup(vpn, &frame)) return 0;
    if (vpn < 0 || vpn >= NPT || ptab[vpn] < 0) return -1; // page fault
    tlb_insert(vpn, ptab[vpn]);
    return 0;
}

int main(void) {
    for (int i = 0; i < NPT; i++) ptab[i] = i + 10; // identity-ish frames
    int trace[] = {0, 1, 2, 3, 0, 1, 4, 0};
    for (int i = 0; i < 8; i++)
        if (access(trace[i]) != 0) { printf("fault?!\n"); return 1; }
    long total = hits + misses;
    long eat = (hits * T_HIT + misses * T_MISS) / total;
    printf("hits=%ld misses=%ld eat=%ld\n", hits, misses, eat);
    return hits != 2 || misses != 6 || eat != 176;
}
```

What this does: replays the Concept trace through a 4-entry FIFO, printing the two counters + EAT your hand-trace predicts.

| Lines | Code | Why it exists |
|---|---|---|
| 3–6 | sizes + costs | 16-page table (all present — isolates *TLB* misses from page faults), 4-entry cache, hit/miss cycle costs (model numbers, stated not smuggled) |
| 8–10 | entries + FIFO hand | `valid` (cold-start empties), `tlb_next` round-robin victim (FIFO: oldest in, first out — LRU approximated in Exercise 3) |
| 12–21 | lookup | linear scan (4 entries — real TLBs are associative hardware; the *semantics* match, the speed doesn't need to) |
| 23–30 | insert | overwrite victim, advance hand, count the miss (insert happens exactly on walk success — fault path inserts nothing) |
| 32–38 | access | TLB → table-or-fault → insert (the hierarchy: cache, backing store, exception — same shape as every cache you'll meet) |
| 42–48 | trace + EAT | fixed array (deterministic — no RNG in simulators, ever), EAT = weighted mean (the number architects quote) |

Change X → Y: change trace to `{0,0,0,0,1,1,1,1}`. Verify: `hits=6 misses=2` (locality rewarded — same cache, opposite behavior; workload *is* the performance).

## Build It

```bash
make run
make test
```

What this does: prints `hits=2 misses=6 eat=176`, then asserts counters, EAT, FIFO-eviction ordering, and a hot-trace high-hit case.

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | the three numbers must match Concept byte-for-byte (hand-trace first, machine confirms) |
| `make test` | machine proof | same engine constants, asserted (EAT math is where architects' spreadsheets die — pin it) |

Change X → Y: grow `NTLB` to 8. Verify: `misses=5` (only cold misses — capacity covers the working set; the knee every capacity plot shows, found by editing one digit).

## Use It (Linux)

Real TLBs report pressure indirectly:

```bash
./build/tlb
perf stat -e dTLB-loads,dTLB-load-misses,iTLB-load-misses ./build/tlb 2>&1 | head -8 || echo "(no perf counters here; EAT math above stands alone)"
getconf PAGESIZE
```

What this does: runs the toy, then asks hardware for *its* TLB miss counts (same counters, silicon edition) plus page size (reach = entries × size).

| Lines | Code | Why it exists |
|---|---|---|
| `perf dTLB-*` | silicon truth | loads vs misses = your hits/misses at GHz (containers often deny counters — the `||` keeps the lesson green regardless) |
| `PAGESIZE` | reach math | 64 entries × 4K = 256 KiB covered (hugepages ×512 the reach per entry — the 2M-page argument, numeric) |

Change X → Y: `perf stat` a stride program (Exercise 2's) vs this loop. Verify: stride misses dwarf loop misses (hardware agrees with the toy — locality, measured twice).

## Ship It

Artifact: `outputs/tlb-card.md` — hit/miss costs, EAT formula, FIFO-vs-LRU note, flush/ASID/hugepage trio, `perf` verbs. Cite it in every "why is this scan slow?" investigation henceforth.

## Exercises

1. Easy — hand-trace `{0,1,2,3,4,5,0,1}` on 4-entry FIFO (expect 0 hits — cyclic thrash bigger than cache; the pathology named).
2. Medium — stride program: 64 pages touched every 16th then sequentially; count misses each way (stride defeats prefetch+TLB — measure both defeats separately).
3. Hard — LRU variant (timestamps per entry, evict oldest-used): show `{0,1,2,3,0,1,4,0}` gains nothing but `{0,1,2,3,4,0,1}`-style loops gain (FIFO vs LRU differ on *cyclic* reuse — find the trace that splits them, the classic exam question answered empirically).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| TLB | cached VPN→frame answers (per-CPU, flushed or ASID-tagged) | [tlb](../../../../glossary/terms.md#tlb) |
| page | 4 KiB unit the cache keys on (huge pages: fewer keys, more reach) | [page](../../../../glossary/terms.md#page) |
| PCB | switch swaps roots → TLB flush (unless ASID); cost of context switch, part 2 | [pcb](../../../../glossary/terms.md#pcb) |

## Further Reading

- OSTEP Ch.19 — TLB algorithms + EAT math (this lesson runs its examples).
- Intel SDM Vol.3 4.10 — caching translation (invalidation rules: the `invlpg` verb).
- `man 1 perf-stat` (`dTLB-loads`, `page-faults` — silicon counters for each software number here).

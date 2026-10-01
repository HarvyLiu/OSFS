# Swap, Demand Paging, OOM — Promises Beyond RAM

> Map more than fits. Fault it in on touch. Evict the cold, write back the dirty, kill when lying fails.

**Type:** Build
**Languages:** C
**Prerequisites:** 02-tlb
**College ref:** OSTEP Ch.21–22 (swap + demand paging), MIT 6.1810 page-fault path notes, Linux `mm/vmscan.c` + OOM killer (policy preview)
**Time:** ~75 minutes

## Learning Objectives
- Trace a 13-access stream through 3 frames (FIFO), counting faults/evictions/writebacks by hand
- Implement demand engine: hit, major fault, clean/dirty evict, numbered outcomes
- Explain dirty tracking (writeback only the changed) and OOM (promises exceed RAM+swap)
- Connect present-bit faults (05/02's -2) to disk loads and to `VmSwap`/`major faults` counters

## Concept in 60s

![swap flow](../figures/swap-flow.svg)

<!-- source: ../figures/swap-flow.excalidraw — open in excalidraw.com to redraw -->

Picture three frames and a workload that names six pages — you promised more than fits. Each first touch faults its page in from disk (a major fault, disk to frame). When the house is full and a new page arrives, you evict the FIFO victim: clean pages drop free since disk already holds them, dirty pages must write back first since disk is stale. A clear present bit means "ask the fault handler," the -2 from 05/02 grown into a loading dock. Our 13-access trace lands at 9 faults, 6 evictions, 1 writeback, resident `{4,2,5}`. And when promises outrun RAM *and* swap, Linux stops loading and starts choosing — the OOM killer picks a victim by badness score, tunable via `oom_score_adj`, and you read the verdict in `dmesg`.

## Simulate It (host C — the engine, no QEMU)

Core: `code/demand.h` + `code/demand.c`. FIFO, dirty bits, numbered outcomes.

```c
// demand.c -- FIFO eviction to swap, dirty writebacks.
#include "demand.h"

void demand_init(demand_t *d) {
    for (int i = 0; i < D_NFRAMES; i++) {
        d->frames[i] = -1;
        d->dirty[i] = 0;
    }
    d->fifo = 0;
    d->faults = d->evictions = d->writebacks = 0;
}

int demand_access(demand_t *d, int vpn, int is_write) {
    if (vpn < 0 || vpn >= D_NPAGES) return -1;
    for (int i = 0; i < D_NFRAMES; i++)
        if (d->frames[i] == vpn) {
            if (is_write) d->dirty[i] = 1;
            return 0;
        }
    d->faults++;
    int v = d->fifo;
    if (d->frames[v] != -1) {
        d->evictions++;
        if (d->dirty[v]) d->writebacks++;
    }
    d->frames[v] = vpn;
    d->dirty[v] = is_write ? 1 : 0;
    d->fifo = (d->fifo + 1) % D_NFRAMES;
    return 1;
}
```

What this does: hits set dirty on write; misses fault, evict (writeback if dirty), load, advance hand — demand paging's whole economy in 20 lines.

| Lines | Code | Why it exists |
|---|---|---|
| 5–11 | zeroed init | `-1` frames = empty (no resident), counters zeroed (statistics start honest) |
| 14 | range guard | `vpn` outside 0–5 = unmapped *address* (-1), distinct from swapped-out (fault+load): addresses vs residency, separated |
| 15–19 | hit path | linear scan (3 frames — real sets associative; semantics match); write stamps dirty (the bit that later forces writeback) |
| 21–29 | miss path | fault++ (major: disk involved, real or modeled); empty slot = free load; occupied = evict (+writeback iff dirty — clean pages drop free, the optimization that halves disk traffic) |

Change X → Y: seed all `dirty=1` at init (all clean→dirty lie). Verify: writebacks climb (every evict writes — the tax of *not* tracking, measured; tracking exists to skip exactly this).

Driver (`code/main.c`) — the calibrated trace:

```c
int trace[][2] = {
    {0,0},{1,0},{2,0},          // fill: 3 faults
    {0,0},{1,0},                // hot: hits
    {3,0},                      // evict 0 (clean)
    {0,1},{1,0},                // 0 back (evict 1), dirty 0
    {4,0},                      // evict 2
    {0,0},{1,0},{2,0},          // 0 hit; 1 back (evict 3); 2 back (evict dirty 0 -> writeback)
    {5,0},                      // evict 4
};
```

What this does: fills, revisits, churns, dirties exactly page 0 once — engineered so each counter has an alibi (below).

| Lines | Code | Why it exists |
|---|---|---|
| fill + hot | baseline | 3 faults then 2 hits (residency works before pressure starts) |
| `{3,0}` + `{0,1}` | first blood + dirt | evict clean 0, reload it *dirty* (the writeback's future victim, planted) |
| tail churn | pressure | 4 evictions including the dirty one (writebacks=1 exactly — count them in the trace: only page 0 was ever written) |

Hand trace (every number's alibi — frames F0,F1,F2, hand→):

```text
0,1,2 miss (F=0,1,2)      faults=3
0,1 hit                    (residency)
3 miss, evict clean 0       faults=4 ev=1   F=3,1,2
0w miss, evict clean 1      faults=5 ev=2   F=3,0*,2  (*=dirty)
1 miss, evict clean 2       faults=6 ev=3   F=3,0*,1
4 miss, evict clean 3       faults=7 ev=4   F=4,0*,1
0 hit; 1 hit
2 miss, evict DIRTY 0       faults=8 ev=5 wb=1  F=4,2,1
5 miss, evict clean 1       faults=9 ev=6   F=4,2,5
```

What this does: proves `faults=9 evictions=6 writebacks=1 resident={4,2,5}` interval by interval — no simulator constant trusted without its line here.

Real output (must match byte-wise):

```text
faults=9 evictions=6 writebacks=1 resident= 4 2 5 majors=9/13
```

OOM honesty (`code/oom.c` — capped allocator loop, deterministic via argv):

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    long mb = argc > 1 ? atol(argv[1]) : 3;
    if (mb < 0 || mb > 256) { fprintf(stderr, "cap 0..256\n"); return 2; }
    char *blocks[256];
    long got = 0;
    for (long i = 0; i < mb; i++) {
        blocks[i] = malloc(1 << 20);
        if (!blocks[i]) break;
        memset(blocks[i], 0xA5, 1 << 20); // touch: fault it in (lazy!)
        got++;
    }
    printf("allocated=%ldMB requested=%ldMB\n", got, mb);
    for (long i = 0; i < got; i++) free(blocks[i]);
    return 0;
}
```

What this does: allocates-then-touches up to N MiB (touch matters: untouched `malloc` may never fault — 05/01's promise/delivery split, weaponized), frees clean, reports both numbers.

| Lines | Code | Why it exists |
|---|---|---|
| `atol(argv[1])` + cap | deterministic knob | tests pass small N (fast, exact); humans try big N under `ulimit -v` to *watch* NULL arrive early |
| `memset` touch | fault it in | allocation promises, touch delivers (untouched pages cost nothing — overcommit's whole trick, visible here) |
| `break` on NULL | honest failure | OOM handled, not crashed (kernel OOM-killer is what happens when *nobody* checks — Use It reads its diary) |

Change X → Y: `./build/oomdemo 3` vs `ulimit -v 4000; ./build/oomdemo 64`. Verify: first prints `3/3`, second prints small-got/64 (address-space cap forces early NULL — OOM reproduced on demand, no machine harmed).

## Build It

```bash
make run
make test
```

What this does: prints the calibrated line + OOM demo, then asserts faults/evictions/writebacks/resident + fault-path(-1) + OOM exact-at-3.

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | `faults=9 evictions=6 writebacks=1 resident= 4 2 5 majors=9/13` + `allocated=3MB requested=3MB` |
| `make test` | machine proof | engine asserts on the real `demand.c` (not copies) + oom determinism at cap 3 |

Change X → Y: change frames 3→4 (edit `D_NFRAMES`, re-trace first!). Verify: faults drop (bigger house, fewer evictions — Belady's curve in one digit; FIFO can *anomaly* on some traces — Exercise 3 hunts it).

## Use It (Linux)

Real swap + OOM diaries:

```bash
cat /proc/meminfo | grep -E "MemTotal|MemFree|SwapTotal|SwapFree"
ps -eo pid,comm,vsz,rss | sort -k4 -n | tail -4
dmesg 2>/dev/null | grep -i "oom" | tail -3 || journalctl -k 2>/dev/null | grep -i oom | tail -3 || echo "(no OOM history: healthy box)"
```

What this does: shows RAM vs swap sizes, top RSS consumers (OOM-killer candidates, ranked), and any past OOM kills (the diary).

| Lines | Code | Why it exists |
|---|---|---|
| `meminfo` | promise pool | Mem+Swap = total promises issuable (overcommit tuning lives in `/proc/sys/vm/overcommit_*`) |
| `ps vsz/rss` | promise vs residence | VSZ (mapped) ≫ RSS (touched) for lazy programs (05/01's split, fleet-wide) |
| OOM diary | the killer's log | `Killed process <name> (score ...)` — badness picked the victim (scores tunable via `/proc/PID/oom_score_adj`, -1000 = never) |

Change X → Y: `cat /proc/self/oom_score`. Verify: small number (your shell is innocent — scores scale with RSS + tuning; watch it while running the 64 MB demo unconstrained).

## Ship It

Artifact: `outputs/swap-card.md` — fault/evict/writeback trio, dirty-bit rule, FIFO-vs-LRU note, OOM score verbs, `meminfo`/`ps` readers. Memory reference complete (05+06): spaces → tables → TLB → swap → killer. You now own demand paging end to end — the block layer in phase 07 gives evicted pages somewhere durable to go.

## Exercises

1. Easy — re-trace with `{0,1}` marked dirty from the start (all evicts write back: count = evictions — tracking's value, priced).
2. Medium — LRU variant (timestamps, evict oldest-used): show our trace gains ≤1 (FIFO-vs-LRU splits on *cyclic* traces — construct one where LRU wins by ≥2, the classic answer, empirically).
3. Hard — Belady hunt: find a trace where 4 frames fault MORE than 3 (FIFO anomaly exists — 0,1,2,3,0,1,4,0,1,2,3,4 family; verify with your engine, then explain why LRU/stack algorithms immune).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| page | unit of residency (present=swap-in, absent=fault-or-hole) | [page](../../../../glossary/terms.md#page) |
| TLB | caches translations (evicted pages need shootdowns — 06/02's coherence, continued) | [tlb](../../../../glossary/terms.md#tlb) |
| heap | `malloc` arenas ride demand paging (untouched = unresident = free) | [heap](../../../../glossary/terms.md#heap) |
| syscall | `mmap`/`brk` grow promises; faults fulfill; OOM kills the unfundable | [syscall](../../../../glossary/terms.md#syscall) |

## Further Reading

- OSTEP Ch.21–22 — swap mechanisms + policies (demand engine's textbook).
- `man 5 proc` (`oom_score`, `meminfo` rows) + `man 1 ps` (VSZ/RSS columns).
- Linux `Documentation/mm/` oom-killer notes (badness heuristics, production-flavored).

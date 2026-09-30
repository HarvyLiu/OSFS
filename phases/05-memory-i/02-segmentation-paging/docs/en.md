# Segmentation + Paging — Two Ways to Lie About Addresses

> Segments add a base. Pages look up a table. Tables won; here's the autopsy and the coronation.

**Type:** Learn
**Languages:** C
**Prerequisites:** 01-address-spaces
**College ref:** OSTEP Ch.15–18 (segmentation → paging), MIT 6.1810 Lec 7 (page tables), xv6 file kernel/vm.c (`walk` preview)
**Time:** ~75 minutes

## Learning Objectives
- Trace base+limit translation and its two faults (over-limit, wrong segment) by hand
- Implement VPN/offset split + flat-table translate with present-bit faults in C
- Explain why paging won (fixed pages kill external fragmentation; present bits enable swap)
- Connect PTE flags (present/RW/user) to Protection-phase enforcement ahead

## Concept in 60s

![vpn offset split](../figures/vpn-offset.svg)

<!-- source: ../figures/vpn-offset.excalidraw — open in excalidraw.com to redraw -->

Segmentation: `phys = base[seg] + offset`, fault if `offset >= limit[seg]` (variable-sized chunks — simple, fragments externally like a bad malloc). Paging: chop the virtual [address](../../glossary/terms.md#address) into `VPN | offset` (32-bit VA, 4 KiB pages → 20-bit VPN + 12-bit offset: `vpn = va >> 12`, `off = va & 0xFFF`): look up `frame = table[vpn]`, fault if `!present`, then `phys = frame*4096 + off`. Fixed-size pages → no external fragmentation; per-[page](../../glossary/terms.md#page) flags (present/read-write/user) → protection + swap for free. The table *is* the address space (per-process root — the field your PCB will carry).

## Simulate It (host C — both translators, no QEMU)

Full program: `code/trans.c`. One segment table, one flat page table, printed walks.

```c
#include <stdio.h>

#define PGSIZE 4096
#define NSEG 3
#define NPT 8

static unsigned seg_base[NSEG] = {0x10000, 0x20000, 0x30000};
static unsigned seg_lim[NSEG] = {0x4000, 0x2000, 0x1000};

// flat page table: frame number, -1 = not present
static int ptab[NPT] = {5, -1, 7, 3, -1, -1, 9, -1};

static long seg_translate(int seg, unsigned off, unsigned *phys) {
    if (seg < 0 || seg >= NSEG) return -1;      // bad segment
    if (off >= seg_lim[seg]) return -2;         // over limit
    *phys = seg_base[seg] + off;
    return 0;
}

static long page_translate(unsigned va, unsigned *phys) {
    unsigned vpn = va >> 12;
    unsigned off = va & 0xFFF;
    if (vpn >= NPT) return -1;                  // outside table
    if (ptab[vpn] < 0) return -2;               // not present
    *phys = (unsigned)ptab[vpn] * PGSIZE + off;
    return 0;
}

int main(void) {
    unsigned p = 0;
    int rc;
    // NOTE: call-then-print. Calling inside printf args prints stale p:
    // C argument order is unspecified. Verification caught this exact bug.
    rc = seg_translate(1, 0x100, &p);
    printf("seg(1,0x100)=%s 0x%x\n", rc == 0 ? "ok" : "FAULT", p);
    rc = seg_translate(1, 0x2000, &p);
    printf("seg(1,0x2000)=%s (limit 0x2000)\n", rc == 0 ? "ok" : "FAULT");
    rc = page_translate(0x1ABC, &p);
    printf("page(0x1ABC)=%s (vpn1 !present)\n", rc == 0 ? "ok" : "FAULT");
    rc = page_translate(0x2ABC, &p);
    printf("page(0x2ABC)=%s 0x%x (frame7+0xABC)\n", rc == 0 ? "ok" : "FAULT", p);
    rc = page_translate(0x6123, &p);
    printf("page(0x6123)=%s 0x%x (frame9+0x123)\n", rc == 0 ? "ok" : "FAULT", p);
    return 0;
}
```

What this does: walks both schemes over hand-picked addresses — good, over-limit, not-present, and cross-page-offset cases, each labeled.

| Lines | Code | Why it exists |
|---|---|---|
| 3–9 | tables | 3 segments (base+limit pairs) + 8-entry flat table (frame or -1): the entire hardware state for both MMUs, toy-sized |
| 11–16 | seg walker | bounds-check segment, limit-check offset, add base (two faults, distinct codes: -1 bad-seg, -2 over-limit — debuggability by numbering) |
| 18–25 | page walker | split (`>>12`/`&0xFFF`), range-check VPN, present-check, recombine (faults -1/-2 mirror seg's — same triage shape, new mechanism) |
| 29–33 | labeled prints | each line names the expectation *in the string* (`vpn1 !present`, `frame7+0xABC`): output reads as its own proof |

Change X → Y: change `ptab[1]` from `-1` to `6`. Verify: `page(0x1ABC)` flips FAULT→ok `0x6ABC` (proves presence is data — the bit Memory II's swapper will clear to evict).

## Build It

```bash
make run
make test
```

What this does: prints the five labeled walks, then asserts translations + all four fault codes + offset preservation (`off` passes through untouched — the key insight tests pin).

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | expect `ok 0x20100`, `FAULT (limit)`, `FAULT (vpn1)`, `ok 0x7abc`, `ok 0x9123` |
| `make test` | machine proof | same walkers, assert values (offset math is where paging bugs hide — `0xABC` must survive both trips) |

Change X → Y: change `PGSIZE` math to `va >> 10` (1 KiB pages) mentally first. Verify by editing + watching `0x2ABC` split differently (page *size* is a parameter — x86 uses 4K/2M/1G; the split moves with it).

## Use It (Linux)

Real boxes page exactly like this (plus levels):

```bash
grep -i "page size" /proc/self/smaps 2>/dev/null | head -2 || getconf PAGESIZE
cat /proc/self/maps | awk '{print $1}' | head -4
```

What this does: reports your page size (4096, canonically) and shows region ranges that are all page-multiples (start/end align — the allocator from P-02/06's `ALIGN` in the wild).

| Lines | Code | Why it exists |
|---|---|---|
| `getconf PAGESIZE` | ground truth | 4096 on x86-64 (the `>>12` in your walker, confirmed by the OS itself) |
| `maps` ranges | alignment proof | every `xxxx-yyy` start/end divisible by 0x1000 (mappings are page-granular — protection flags attach per page, next lesson's PTE bits) |

Change X → Y: `python3 -c "print(hex(0x7f8b2c1ab000 & 0xFFF))"` on any maps start. Verify: `0x0` (low 12 bits zero = page-aligned — the offset field, observed live).

## Ship It

Artifact: `outputs/translate-card.md` — seg formula + faults, VPN/offset split for 4K/2M/1G, present/RW/U bit meanings, `>>12`/`&0xFFF` verbs. The pocket reference for 05/03's multi-level walk (same split, applied thrice).

## Exercises

1. Easy — translate `0x3456` and `0x7FFF` by hand through `ptab`, then `make run` to confirm (hand before machine, always).
2. Medium — add per-entry RW bit array; writes to read-only entries fault -3 (Protection phase in one flag — implement the check + tests).
3. Hard — external fragmentation demo: segments of sizes {8K,8K,8K} in 20K RAM with churn (alloc/free random, first-fit): show a 8K alloc failing despite 12K free (holes don't merge — the autopsy exhibit for why pages won).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| page | 4 KiB translation atom (VPN→frame + offset passthrough) | ../../glossary/terms.md#page |
| address | split into VPN\|offset before lookup (never translated whole) | ../../glossary/terms.md#address |
| TLB | cache of recent translations (why walks don't run per access — 06 Memory II) | ../../glossary/terms.md#tlb |

## Further Reading

- OSTEP Ch.15–18 — segmentation → paging → TLBs (the trilogy this lesson compresses).
- xv6 `kernel/vm.c:walk` (skim) — the multi-level version of `page_translate` (next lesson implements it).
- Intel SDM Vol.3 Ch.4 — paging structures reference (one figure: the 4-level walk).

# Address Spaces — Everyone Gets Their Own Private Infinity

> Your pointers start at zero and end at forever. Everyone else's do too. The MMU keeps the stories straight.

**Type:** Learn
**Languages:** C
**Prerequisites:** 01-abstractions
**College ref:** OSTEP Ch.13 (address spaces), MIT 6.1810 Lec 7 (virtual memory), xv6 file kernel/memlayout.h (the map, preview)
**Time:** ~60 minutes

## Learning Objectives
- Trace one process's regions (text/data/bss/heap/mmap/stack) using its own printed pointers
- Implement an `mmap` anonymous mapping (Linux) and read it back through the page cache's absence
- Explain virtual vs physical, 4 KiB pages, and why same-numbered pointers differ across processes
- Connect the map to paging (next: the tables that implement it) and OOM (what happens when lies exceed RAM)

## Concept in 60s

![address space map](../figures/address-space.svg)

<!-- source: ../figures/address-space.excalidraw — open in excalidraw.com to redraw -->

Low addresses: code+data (your ELF's sections, P-02/06). Then heap (grows up via `brk`/`mmap`). Then mmap gaps (shared libs, big allocs). High: stack (grows down) + kernel half (top, yours to trap into, never to touch). Every process sees this *same shape* with *different contents* — virtual numbers translated per-process by page tables (4 KiB pages: the atom of honesty) into physical frames. `mmap(ANONYMOUS)` asks the lie directly: "give me N zeroed pages, nowhere-file-backed." `/proc/self/maps` prints your space live. See [address](../../glossary/terms.md#address), [heap](../../glossary/terms.md#heap), [page](../../glossary/terms.md#page).

## Simulate It (host — selfie first, portable)

Full program: `code/layout.c`. Prints each region's address from inside.

```c
#include <stdio.h>
#include <stdlib.h>

int init_global = 41;
int zero_global;

int main(void) {
    int stack_var = 1;
    void *heap1 = malloc(16);
    void *heap2 = malloc(16);
    if (!heap1 || !heap2) return 1;
    printf("code(main)=%p rodata=%p data=%p bss=%p\n",
           (void *)main, (void *)"lit", (void *)&init_global, (void *)&zero_global);
    printf("heap1=%p heap2=%p gap=%ld\n", heap1, heap2,
           (long)((char *)heap2 - (char *)heap1));
    printf("stack~%p heap-below-stack=%d code-below-heap=%d\n",
           (void *)&stack_var, (void *)&stack_var > heap1, heap1 > (void *)main);
    free(heap1);
    free(heap2);
    return 0;
}
```

What this does: exhibits code→data→heap→stack ordering from the inside — P-02/06's sections plus P-01/04's heap plus a stack anchor, one_panorama.

| Lines | Code | Why it exists |
|---|---|---|
| 4–5 | data/bss pair | initialized vs zeroed globals (linker classes, now addressed — compare with `nm` letters from P-02/06) |
| 8–9 | two small mallocs | adjacent-ish heap chunks; `gap` shows allocator metadata+alignment overhead (not exactly 16 — the allocator's tax, visible) |
| 11–12 | string literal | `"lit"` lives in `.rodata` (read-only — try writing it and meet SIGSEGV, Protection preview) |
| 14–15 | ordering booleans | prints whether stack>heap>code *on this machine*: Linux-canonical `1 1`, Windows differs (regions universal, order OS-specific — the diagram shows Linux) |

Change X → Y: `malloc(16)` → `malloc(1000000)` for heap2. Verify: gap explodes (big allocs take separate `mmap` regions, not the heap — the allocator's two-gear secret, Use-It-verifiable in maps).

## Build It (mmap the lie directly — Linux/WSL/Docker)

`code/map.c` asks for raw pages (needs `<sys/mman.h>` — Linux-only, like `fork` before it):

```c
// map.c -- anonymous pages, zeroed, mine. Lesson docs/en.md. Linux-only.
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
    long ps = sysconf(_SC_PAGESIZE);
    size_t len = (size_t)ps * 2;
    char *p = mmap(0, len, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); return 1; }
    p[0] = 'A';
    p[len - 1] = 'Z';
    printf("pagesize=%ld len=%zu map=%p first=%c last=%c\n", ps, len, (void *)p, p[0], p[len - 1]);
    munmap(p, len);
    return ps != 4096; // x86-64 Linux norm; noted, not worshipped
}
```

What this does: maps 2 anonymous pages (zeroed by the kernel — no file, no malloc), touches both ends (faults them in — lazily!), prints, releases.

| Lines | Code | Why it exists |
|---|---|---|
| 6 | `sysconf(PAGESIZE)` | page size is *queried*, not assumed (4 KiB on x86-64; hugepages later — query beats lore) |
| 8–9 | `mmap(0, ..., ANONYMOUS)` | `0` = kernel picks address; `PRIVATE\|ANONYMOUS` = zeroed, mine, fileless (the heap's big brother and malloc's backstage) |
| 11 | `MAP_FAILED` check | mmap's -1-style honesty (same contract family as `open`'s -1 — P-01/06's error-shape trilogy) |
| 12–13 | touch ends | first touch *faults* pages in (allocation is a promise; touch is delivery — lazy binding, Memory II deepens) |
| 15 | `munmap` | give back (mmap without munmap = address-space leak — virtual exhaustion is real on 32-bit) |

Change X → Y: remove the two touches, rerun under `/usr/bin/time -v` (Linux). Verify: minor faults drop by ~2 (untouched pages never fault in — proof allocation ≠ residence).

Makefile builds `map` on Linux only (conditional — P-01/06's makefile lesson, grown up):

```make
UNAME := $(shell uname)
all: $(BUILD)/layout
ifeq ($(UNAME),Linux)
all: $(BUILD)/map
endif
```

What this does: `layout` everywhere (portable selfie), `map` where `<sys/mman.h>` exists (Windows builds stay green, Linux builds prove pages).

## Use It (Linux)

Read your space from inside and out:

```bash
./build/layout
pmap -x $$ | head -8
cat /proc/self/maps | awk '{print $1, $6}' | head -8
```

What this does: prints the selfie, then your *shell's* regions with sizes (`pmap -x`), then raw map lines (offsets → pathnames).

| Lines | Code | Why it exists |
|---|---|---|
| `pmap -x` | sized regions | `Kbytes/RSS/Dirty` per mapping (heap vs stack vs libc-so λ³— the selfie with measurements) |
| `/proc/self/maps` | ground truth | `address-range perms offset dev inode pathname` (P-01/06's forensics, now per-region; `[heap]`/`[stack]` labeled by the kernel itself) |

Change X → Y: `pmap -x` on the `map` demo mid-`sleep` (add `sleep(60)` scratch? or `watch pmap`). Verify: a 2-page anonymous region appears exactly `len` big (your `mmap` visible from outside — inside/outside agreement, the whole lesson in one check).

## Ship It

Artifact: `outputs/vm-card.md` — region order, page facts (4 KiB, fault-on-touch), `mmap`/`munmap`/`mprotect` verbs, `pmap`/`maps` readers, virtual-vs-physical one-liner. The Memory-phase passport: stamped at every border crossing ahead.

## Exercises

1. Easy — print `&argc`-family? No: print addresses of 3 nested-call locals (depth.c from P-02/02, rerun) and stack direction (down — the selfie includes growth).
2. Medium — `mmap` 1 GiB anonymous, touch 1 byte per page, `time -v` minor faults (≈262k — laziness quantified: promised gigabytes, faulted kilobytes).
3. Hard — two processes `mmap` the *same* file `SHARED`, both write different offsets, both read all (shared memory without threads — the phase bridge from concurrency to persistence, in 30 lines).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| address | virtual number; translated per-process in 4 KiB pages | ../../glossary/terms.md#address |
| page | 4 KiB translation atom (the honest unit under all lies) | ../../glossary/terms.md#page |
| heap | one region among many (grows up; big allocs bypass via mmap) | ../../glossary/terms.md#heap |
| syscall | `mmap`/`munmap`/`mprotect`/`brk`: the space-shaping traps | ../../glossary/terms.md#syscall |

## Further Reading

- OSTEP Ch.13–14 — spaces + API (the two chapters this lesson executable-izes).
- `man 2 mmap,munmap,mprotect` + `man 1 pmap` — verbs + readers.
- xv6 `kernel/memlayout.h` — one file, the whole map (every constant now parses).

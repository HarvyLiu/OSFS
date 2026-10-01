# malloc/free — Owning Heap Memory on Purpose

> Stack memory borrows. Heap memory owns. Every leak is a forgotten promise.

**Type:** Build
**Languages:** C
**Prerequisites:** 03-c-pointers
**College ref:** OSTEP Ch.13–14 (address space + allocator API), MIT 6.1810 kalloc (what malloc becomes), xv6 file kernel/kalloc.c (free-list preview)
**Time:** ~75 minutes

## Learning Objectives
- Trace malloc → use → realloc-grow → free as an ownership lifecycle with NULL checks
- Implement a growable int vector from raw `malloc`/`realloc`/`free` (no hidden magic)
- Explain leaks, double-free, and use-after-free as the three heap sins (with ASan proof)
- Connect user `malloc` to kernel `kalloc`/`kfree` (page-granular cousins in later phases)

## Concept in 60s

![heap map](../figures/heap-map.svg)

<!-- source: ../figures/heap-map.excalidraw — open in excalidraw.com to redraw -->

[Stack](../../../../glossary/terms.md#stack) frames die at `}`. The [heap](../../../../glossary/terms.md#heap) lives until *you* free it — across calls, across files, until process exit (or forever, if you leak). `malloc(n)` asks for `n` bytes, returns an [address](../../../../glossary/terms.md#address) or `NULL`. `realloc` grows (maybe moving). `free` gives back. Ownership rule: exactly one owner frees, exactly once, never touches after. Break it three ways: leak (never free), double-free (free twice → heap metadata corrupt), use-after-free (touch after free → somebody else's bytes now).

## Simulate It (host, no QEMU)

Full program: `code/heap.c` — a tiny vector that grows 4 → 8, prints, frees.

```c
#include <stdio.h>
#include <stdlib.h>

typedef struct { int *data; unsigned len, cap; } vec_t;

static int vec_push(vec_t *v, int x) {
    if (v->len == v->cap) {
        unsigned nc = v->cap ? v->cap * 2 : 4;
        int *nd = realloc(v->data, nc * sizeof(int));
        if (!nd) return -1;
        v->data = nd;
        v->cap = nc;
    }
    v->data[v->len++] = x;
    return 0;
}

int main(void) {
    vec_t v = {0};
    for (int i = 0; i < 6; i++)
        if (vec_push(&v, i * 10) != 0) { fprintf(stderr, "oom\n"); return 1; }
    printf("len=%u cap=%u vals=%d %d %d...\n", v.len, v.cap, v.data[0], v.data[1], v.data[2]);
    free(v.data);
    v.data = 0;
    return 0;
}
```

What this does: owns a resizable array end-to-end — allocate on demand, double when full, release once — the pattern behind every PCB table and buffer cache later.

| Lines | Code | Why it exists |
|---|---|---|
| 4 | `vec_t` | length + capacity + pointer: length = used, capacity = owned (the invariant every vector bug violates) |
| 7–14 | grow branch | full? double (`*2` amortizes to O(1) per push); start at 4 so first push allocates |
| 9 | `realloc(...)` | grow-or-move: contents copied, old freed *by realloc*; returns `NULL` on failure with old block still valid (never assign blindly) |
| 10 | `if (!nd)` | out-of-memory path: report, don't crash silently (kernels panic or reclaim here) |
| 16 | `vec_t v = {0}` | zero-init: `data=NULL, len=cap=0` so first `realloc(NULL,...)` behaves like `malloc` |
| 21–22 | `free` + null | give back exactly once, then null the pointer so a stray second `free` crashes loud instead of corrupting quiet |

Change X → Y: push `20` items instead of 6. Verify: `make run` shows `len=20 cap=32` (4→8→16→32 doublings — count them, that's amortized growth working).

Sin gallery (run each under ASan, read the scream — then never ship one):

```bash
cc -fsanitize=address,undefined -std=c11 code/sins.c -o /tmp/sins-leak && /tmp/sins-leak; echo "exit=$?"
```

What this does: builds the companion `sins.c` (one sin per argv: `leak`, `doublefree`, `uaf`) with instruments that turn silent corruption into loud, located aborts.

| Lines | Code | Why it exists |
|---|---|---|
| `-fsanitize=address,undefined` | guards | every load/store checked; leaks reported at exit, double-free/uaf abort at the line |
| `argv` picks the sin | isolation | one bug at a time reads clean; combined bugs read as noise |
| `exit=$?` | proof | leak exits 0-but-reports (still a bug!); double-free/uaf exit non-zero via abort |

Change X → Y: run `/tmp/sins-leak leak` vs `doublefree` vs `uaf`. Verify: three *different* ASan reports (direct-missing-free vs heap-use-after-free vs attempting double-free — learn each headline once, recognize forever).

## Build It (strict + sanitized — the two builds every heap lesson gets)

```bash
make run
make asan && ./build/heap-asan
```

What this does: strict build proves clean under `-Wall -Werror -Wextra`; ASan build proves clean under runtime guards too.

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | compile + run | expect `len=6 cap=8 vals=0 10 20...` (6 items overflowed 4 → doubled to 8) |
| `make asan` | guarded twin | same program + instrumentation; silence here means no hidden sin (valgrind is the slower, deeper alternative) |

Change X → Y: comment out `free(v.data);`. Verify: `make run` looks fine but `make asan` reports `detected memory leaks` with the allocation stack — the proof that "runs fine" never means "owns correctly".

## Use It (Linux)

Real heaps are observable:

```bash
./build/heap
grep -i heap /proc/self/maps
/usr/bin/time -v ./build/heap 2>&1 | grep -E "Maximum resident|Minor|Major"
```

What this does: runs your vector, shows where libc put its heap (`[heap]` line), and reports peak RSS + page faults for 6 ints (baseline for later allocator benchmarks).

| Lines | Code | Why it exists |
|---|---|---|
| `grep -i heap /proc/self/maps` | find the heap | `cat`'s own `[heap]` range — your `malloc` bytes live in a region like this |
| `time -v` | cost it | `Maximum resident` = peak KB; `Minor faults` = first-touch page zeroing (Memory phases will explain each fault) |

Change X → Y: push 6 → 600000 items (edit loop bound). Verify: `Maximum resident` grows ~MBs and `cap` doublings print longer — the moment heap growth becomes a *capacity plan*, not an accident.

## Ship It

Artifact: `outputs/heap-card.md` — ownership trilogy (one malloc→one free, null-after-free, check every return) + ASan/valgrind one-liners. Tape next to the C-loop card; both stay valid to capstone.

## Exercises

1. Easy — print `data` pointer before/after growth to 20 items. Note if it moved (realloc may relocate — never keep old copies).
2. Medium — add `vec_pop` returning last item (don't shrink). Prove `len` drops but `cap` stays (slack is reusable, not leaked).
3. Hard — implement `vec_reserve(n)` (pre-grow) + `vec_shrink` (exact-fit realloc). Benchmark 100k pushes naive vs reserved via `time -v` (this *is* allocator-aware engineering).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| heap | owned-until-freed bytes; you are the lifetime manager | [heap](../../../../glossary/terms.md#heap) |
| pointer | the address malloc returns; NULL = ask failed | [pointer](../../../../glossary/terms.md#pointer) |
| address | heap ranges show in `/proc/*/maps` as `[heap]` | [address](../../../../glossary/terms.md#address) |

## Further Reading

- `man 3 malloc,realloc,free` — exact NULL/move/double-free contracts (read once, cite forever).
- OSTEP Ch.14 — allocator API thinking behind `vec_push`'s doubling.
- xv6 `kernel/kalloc.c` (skim) — free-list of 4 KiB pages; same own/free discipline, page granularity.

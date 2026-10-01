# Memory Map — Static, Stack, Heap: Who Lives Where

> Globals live forever. Locals borrow a frame. malloc'd bytes live until you say otherwise.

**Type:** Learn
**Languages:** C
**Prerequisites:** 02-how-programs-run
**College ref:** OSTEP Ch.13–14 (address-space regions preview), Bryant & O'Hallaron Ch.9 (virtual memory preview), P-02/06 + 05/01 (the deep versions ahead)
**Time:** ~60 minutes

## Learning Objectives
- Name the residents (code/text, static init/zero, heap, stack) and their lifetimes using printed addresses
- Implement one variable per region and show who survives function return
- Explain why returning a local's address breaks (dangling frame) while statics/heap survive
- Connect regions to later lessons (linker places statics, frames nest, heap needs owners)

## Concept in 60s

![memory regions](../figures/memory-regions.svg)

<!-- source: ../figures/memory-regions.excalidraw — open in excalidraw.com to redraw -->

Four neighborhoods: **code/text** (your functions, read-only residents), **static** (globals: initialized in `.data`, zeroed in `.bss` — born at load, die at exit), **[heap](../../../../glossary/terms.md#heap)** (`malloc` arena: lives until `free`), **[stack](../../../../glossary/terms.md#stack)** (frames: locals born at `{`, die at `}` — returning `&local` hands out a corpse's address). Same lesson as P-01/04's ownership and P-02/02's frames, now as *geography*: lifetime questions become "which neighborhood?" questions. (Virtual-vs-physical + paging come in 05 — here, regions and rules suffice.)

## Simulate It (host C — one resident per region, portable)

Full program: `code/regions.c`. Prints addresses grouped by neighborhood.

```c
#include <stdio.h>
#include <stdlib.h>

int g_init = 41;
int g_zero;

int *make_heap(void) {
    int *p = malloc(sizeof *p);
    if (p) *p = 7;
    return p;
}

int *make_mistake(void) {
    int local = 9;
    return &local;  // BUG on purpose (warning expected!)
}

int main(void) {
    int stack_var = 1;
    static int s_persistent = 2;
    int *h = make_heap();
    printf("code=%p g_init=%p g_zero=%p heap=%p stack=%p static-fn=%p\n",
           (void *)main, (void *)&g_init, (void *)&g_zero, (void *)h,
           (void *)&stack_var, (void *)&s_persistent);
    printf("heap-val=%d s_persistent=%d\n", h ? *h : -1, s_persistent);
    free(h);
    printf("mistake-ptr=%p (dangling: NEVER dereference)\n", (void *)make_mistake());
    return h == 0;
}
```

What this does: exhibits all four neighborhoods' addresses plus the two lifetime transfers (heap ownership out, dangling frame out) — geography you can point at.

| Lines | Code | Why it exists |
|---|---|---|
| 4–5 | `.data`/`.bss` pair | initialized vs zeroed globals (P-02/06's sections, now as *lifetimes*: both live whole-run) |
| 7–11 | `make_heap` | returns heap [address](../../../../glossary/terms.md#address) (valid after return — ownership transfers to caller, P-01/04's rule in action) |
| 13–16 | `make_mistake` | returns frame address (dead at `}` — the bug; compiler *warns* `return-local-addr`: read it, never silence it) |
| 19 | function-`static` | local scope + global lifetime (the hybrid: sees-like-local, lives-like-global — singletons wear this) |
| 21–26 | address parade + values | order varies by OS (05/01's lesson: regions universal, order OS-specific — compare with a friend's output!) |
| 27 | print-don't-touch | dangling pointer *printed* (safe) never *dereferenced* (ASan would scream — Exercise 2 does it caged) |

Change X → Y: dereference the mistake (`*make_mistake()`) under ASan (`-fsanitize=address`). Verify: `stack-use-after-return` abort naming the line (the corpse identified forensically — then revert; some things you prove once).

## Build It

```bash
make run 2>&1 | head -8
make test
```

What this does: builds (expect the `return-local-addr` *warning* — kept visible on purpose, not `-Werror`'d away here), runs the parade, asserts region rules.

| Lines | Code | Why it exists |
|---|---|---|
| `2>&1 \| head` | warning shown | the one warning we *keep*: `warning: function returns address of local variable` (read it aloud once — then never ship it) |
| `make test` | machine proof | heap/pointer/static rules asserted (dangling asserted only as *inequality* — never dereferenced, even in tests) |

Change X → Y: add `-Werror` to the build. Verify: build *fails* on the warning (the normal course rule restored — this lesson is the licensed exception, documented here).

## Use It (Linux)

Neighborhoods observable from outside:

```bash
./build/regions | head -3
nm build/regions | grep -E " (g_init|g_zero|main)$"
cat /proc/self/maps | awk '{print $1, $6}' | head -6
```

What this does: runs the parade, resolves symbols to sections (`D`/`B`/`T` — P-02/06's alphabet, your variables), and shows region ranges (05/01's map, previewed).

| Lines | Code | Why it exists |
|---|---|---|
| `nm` trio | letters | `D g_init` (data), `B g_zero` (bss), `T main` (text) — storage class *is* section destiny (P-02/06 again, personal now) |
| `maps` ranges | outside view | `[heap]`/`[stack]` labeled lines (the kernel names your neighborhoods back to you) |

Change X → Y: `s_persistent` — find it in `nm` output (mangled static name, e.g. `s_persistent.0`). Verify: letter `b`/`d` (function-statics live with globals — scope ≠ neighborhood, the distinction this lesson exists to teach).

## Ship It

Artifact: `outputs/regions-card.md` — four neighborhoods (lifetime + section + example), dangling-frame rule, ASan verbs. The lifetime pocket reference through capstone (every use-after-free is a neighborhood violation).

## Exercises

1. Easy — add a `const int LIM = 100`, find it in `nm` (`R` = rodata — constants live in read-only, Protection preview).
2. Medium — ASan-cage the dereference (`-fsanitize=address`, print `*make_mistake()`): paste the `stack-use-after-return` report, map each frame to a lesson (P-02/02's frames, forensically).
3. Hard — `static` counter function called 3× (prints 1,2,3): persistence across calls *without* globals (stateful functions, the pattern behind allocators' arenas and RNGs).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| stack | frames: born at `{`, die at `}` (never return their addresses) | [stack](../../../../glossary/terms.md#stack) |
| heap | malloc arena: lives until `free` (ownership transfers on return) | [heap](../../../../glossary/terms.md#heap) |
| address | neighborhood by number range (order OS-specific, membership universal) | [address](../../../../glossary/terms.md#address) |

## Further Reading

- Bryant & O'Hallaron Ch.9.6 (memory mapping preview — regions with permissions).
- `man 1 nm` (letters again — now with your own symbols).
- 05/01 re-read after this (virtual memory = these regions + translation).

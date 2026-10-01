# Headers, Source Split, Makefile — Code That Scales Past One File

> Headers promise. Sources deliver. Make remembers what changed.

**Type:** Build
**Languages:** C
**Prerequisites:** 05-structs-pcb
**College ref:** OSTEP Ch.1 (building systems from parts), MIT 6.1810 Makefile (multi-file kernel builds), xv6 file Makefile (header deps preview)
**Time:** ~75 minutes

## Learning Objectives
- Trace `#include` as textual paste plus include-guards as double-paste insurance
- Implement a 3-file program (header + two sources) with separate compilation
- Explain object files, linking, and why Make rebuilds only stale targets via timestamps
- Connect header/source split to kernel organization (one `.h` per subsystem, like xv6)

## Concept in 60s

![build graph](../figures/build-graph.svg)

<!-- source: ../figures/build-graph.excalidraw — open in excalidraw.com to redraw -->

`vec.h` *declares* ("there exists `vec_push` taking..."); `vec.c` *defines* (the bytes); `main.c` *uses* (calls it). `#include "vec.h"` pastes the promise into each user. Guards (`#ifndef VEC_H`) stop double-paste when headers include headers. `cc -c` compiles each `.c` to a `.o` (machine code, unresolved calls dangling); the link step resolves them into one binary. `make` compares timestamps: rebuild a `.o` only if its `.c`/`.h` is newer than it — one-line change, one compile, not all. That's the whole build system behind every kernel.

## Simulate It (host — the split, no QEMU)

Three files in `code/`: `vec.h`, `vec.c`, `main.c`. This *is* the program; the lesson is its shape.

```c
// vec.h -- promises + guards. Included by vec.c AND main.c.
#ifndef OSFS_VEC_H
#define OSFS_VEC_H

typedef struct { int *data; unsigned len, cap; } vec_t;
int vec_push(vec_t *v, int x);
void vec_free(vec_t *v);

#endif
```

What this does: publishes the type + two function signatures with zero code — callers compile against promises, linker fulfills them later.

| Lines | Code | Why it exists |
|---|---|---|
| 2–3 | `#ifndef/#define` | include guard: second paste in one translation unit becomes empty (headers including headers would otherwise redefine `vec_t`) |
| 6 | `typedef struct...` | shared layout: both files must agree byte-for-byte or field writes corrupt (one definition of truth, pasted twice) |
| 7–8 | declarations (`;`, no body) | "exists somewhere": compiler emits a *call*, linker later patches the [address](../../../../glossary/terms.md#address) |
| 10 | `#endif` | closes the guard (forget it and nothing compiles — first error points at the *next* file, classic confusion) |

Change X → Y: delete the `#ifndef` line (keep the rest). Verify: `make` fails with `redefinition of vec_t` (proves the guard was load-bearing — `vec.c` includes the header *and* `main.c` does, and the link sees both... actually the error fires at compile of either file that transitively includes twice; restore and it passes).

```c
// vec.c -- delivery. One owner of the logic.
#include <stdlib.h>
#include "vec.h"

int vec_push(vec_t *v, int x) {
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

void vec_free(vec_t *v) {
    free(v->data);
    v->data = 0;
    v->len = v->cap = 0;
}
```

What this does: owns the doubling-growth logic from P-01/04 plus a nulling free — one implementation, many callers.

| Lines | Code | Why it exists |
|---|---|---|
| 3 | `#include "vec.h"` | quotes = local file (brackets `<>` = system); self-checks the definition matches its own promise |
| 5–14 | `vec_push` | same amortized doubling as P-01/04 (ownership rule unchanged — now the *module* owns it) |
| 16–20 | `vec_free` | free + null + zero counts: use-after-free becomes loud NULL crash instead of quiet heap reuse |

Change X → Y: change `free(v->data)` to nothing (leak on purpose). Verify: `make asan` reports the leak with `vec_free` on the stack (proves ownership lives in exactly one function — audit that one to fix all callers).

```c
// main.c -- user. Sees promises only.
#include <stdio.h>
#include "vec.h"

int main(void) {
    vec_t v = {0};
    for (int i = 0; i < 6; i++)
        if (vec_push(&v, i * 10) != 0) return 1;
    printf("len=%u cap=%u first=%d last=%d\n", v.len, v.cap, v.data[0], v.data[5]);
    vec_free(&v);
    return 0;
}
```

What this does: drives the module through its public surface only — never touches `realloc` directly (encapsulation you can enforce by code review: only `vec.c` may).

| Lines | Code | Why it exists |
|---|---|---|
| 6 | `vec_t v = {0}` | zero-init contract (first push allocates — the promise `vec.h` implies, `vec.c` honors) |
| 7–8 | push loop + OOM check | caller handles `-1` (P-01/04 discipline, now across a module boundary) |
| 10 | `vec_free(&v)` | exactly one free, then `main` must not touch `v.data` (null helps, discipline decides) |

Change X → Y: call `vec_push` 20 times. Verify: `len=20 cap=32` (doubling math survived the split — modules don't change arithmetic).

## Build It (separate compile + timestamp rebuilds)

```make
CC=cc
CFLAGS=-Wall -Werror -Wextra -std=c11
BUILD=build
all: $(BUILD)/split
$(BUILD)/split: main.c vec.c vec.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) main.c vec.c -o $(BUILD)/split
run: $(BUILD)/split
	./$(BUILD)/split
```

What this does: one-command build+run; the dependency list (`main.c vec.c vec.h`) is the timestamp contract in miniature.

| Lines | Code | Why it exists |
|---|---|---|
| `$(BUILD)/split: main.c vec.c vec.h` | target: prerequisites | `make` rebuilds iff any prerequisite is *newer* than the target (touch one file, watch one rebuild) |
| `$(CC) ... main.c vec.c` | compile + link together | fine at 2 files; at 200 (xv6) you'd compile `-c` to `.o` per file then link (Exercise 3) |

Change X → Y: run `touch vec.h && make -n run`. Verify: `make -n` (dry run) prints the rebuild commands (proves header edits invalidate — headers are *source*, not comments).

Object-level view (what kernels actually do):

```bash
cc -Wall -Werror -std=c11 -c vec.c -o build/vec.o
cc -Wall -Werror -std=c11 -c main.c -o build/main.o
cc build/vec.o build/main.o -o build/split2
nm build/vec.o | grep -E "vec_push|vec_free"
```

What this does: compiles each unit separately, links the pair, proves the symbols crossed the object boundary.

| Lines | Code | Why it exists |
|---|---|---|
| `-c` twice | separate compilation | each `.o` holds machine code + *unresolved* calls (`vec_push` dangles in `main.o`) |
| link `.o` pair | resolution | linker patches dangling calls to real addresses; `nm` shows `T vec_push` (defined) vs `U` (used-elsewhere) |
| `nm ... grep` | proof | `T` in `vec.o` = promise kept; `U vec_push` in `main.o` = promise used |

Change X → Y: delete `vec.c` from the link line. Verify: `undefined reference to vec_push` (the linker's entire job in one error — declarations compile, definitions link).

## Use It (Linux)

Watch real multi-file builds behave identically:

```bash
touch vec.h && make -n run | head -5
ls -la --time-style=full-iso build/ 2>/dev/null || ls -la build/
echo '#include <stdio.h>' | gcc -E - | head -20
```

What this does: dry-runs the stale rebuild, inspects binary timestamps (make's clock), and shows `#include` as pure paste via the preprocessor.

| Lines | Code | Why it exists |
|---|---|---|
| `make -n` | plan, no action | read what *would* rebuild before spending seconds (kernels: minutes) |
| `ls --time-style` | see staleness | compare `.o` vs `.c` mtimes — the numbers `make` compares |
| `gcc -E` | preprocessor only | 20 lines of pasted headers from one line: includes are textual, not magical |

Change X → Y: run `make run` twice with no edits. Verify: second prints nothing to rebuild (`make: ... up to date` — idempotence is the feature).

## Ship It

Artifact: `outputs/makefile-template` — parameterized 2-file starter (swap `split` for your name, add sources to one line). Plus `outputs/header-template.h` (guard + typedef + decl pattern). Copy both into every later multi-file lesson.

## Exercises

1. Easy — add `vec_pop` (decl in `.h`, def in `.c`, use in `main`). Show `make -n` rebuilds both `.o` equivalents (or the single target) after touching only `.h`.
2. Medium — split objects properly: `vec.o` + `main.o` targets with header deps in the Makefile. Prove `touch main.c` rebuilds only `main.o` + link.
3. Hard — circular include: make `a.h` include `b.h` and vice versa with guards, observe it compiles but forward declarations were needed for pointer members (this *is* why kernels use `struct pcb;` forward decls — preview, implement one).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| address | linker patches call sites with callee addresses across objects | [address](../../../../glossary/terms.md#address) |
| heap | `vec` data still heap-owned; split changes organization, not lifetime | [heap](../../../../glossary/terms.md#heap) |
| freestanding | kernels compile `-ffreestanding` per file the same way; only the link differs (linker script) | [freestanding](../../../../glossary/terms.md#freestanding) |

## Further Reading

- `man 1 make` + `man 1 gcc` (`-c`, `-E`, `-M` for auto-deps).
- xv6 `Makefile` — one rule pattern building ~30 kernel files (read after Exercise 2; it will parse).
- `man 1 nm` — `T`/`U`/`b` letters demystified.

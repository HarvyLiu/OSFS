# C Pointers — Addresses, Dereference, Arithmetic

> A pointer is just an address written down so you can go back to it later.

**Type:** Learn
**Languages:** C
**Prerequisites:** None
**College ref:** OSTEP Ch.13 Address Spaces (intuition), MIT 6.1810 Lec 1 (C refresher), xv6 file kernel/kalloc.c (why pointers matter)
**Time:** ~90 minutes

## Learning Objectives
- Trace what `&x`, `*p`, `p+1` do using a memory map
- Implement pointer dereference and arithmetic from scratch in hosted C
- Explain why `p+1` moves by `sizeof(*p)`, not by 1 byte
- Connect pointers to OS ideas: PCB lists, page tables, heap allocators

## Concept in 60s

![pointer map](../figures/pointer-map.svg)

<!-- source: ../figures/pointer-map.excalidraw — open in excalidraw.com to redraw -->

Think of RAM as numbered mailboxes. `int x = 42;` puts `42` in some mailbox, say #1000. `int *p = &x;` writes `#1000` on a slip of paper called `p`. `*p` means "open the mailbox whose number is on the slip". `p+1` means "next mailbox that fits an `int`", i.e. #1004 if `int` is 4 bytes — not #1001. Every OS structure (see [pointer](../../glossary/terms.md#pointer), [address](../../glossary/terms.md#address), [heap](../../glossary/terms.md#heap)) is built from this trick.

## Simulate It (host, no QEMU)

Full program: `code/main.c`. Build with `make run`. No QEMU, no root needed.

```c
#include <stdio.h>

int main(void) {
    int x = 42;
    int *p = &x;
    printf("x=%d *p=%d\n", x, *p);
    *p = 99;
    printf("after *p=99: x=%d\n", x);
    printf("p points at %p, next int would be %p\n",
           (void *)p, (void *)(p + 1));
    return 0;
}
```

What this does: stores 42, points at it, reads through the pointer, writes through the pointer, then prints addresses to show arithmetic.

| Lines | Code | Why it exists |
|---|---|---|
| 1 | `#include <stdio.h>` | brings in `printf`; hosted C only (kernels can't use this) |
| 4 | `int x = 42;` | reserves 4 bytes on the [stack](../../glossary/terms.md#stack), fills with 42 |
| 5 | `int *p = &x;` | `&x` = address of x; `int *` = "slip of paper holding an int's address" |
| 6 | `printf(... *p ...)` | `*p` = go to address in `p` and read the int there; proves `p` points at `x` |
| 7 | `*p = 99;` | go to address in `p` and overwrite with 99; `x` changes too (same mailbox) |
| 9–10 | `(void *)(p+1)` | `p+1` advances by `sizeof(int)`; cast to `void*` so `%p` prints cleanly |

Change X → Y: change `*p = 99;` to `*p = 7;`. Verify: `make run` → expected second line `after *p=99: x=7` becomes `x=7` (proves write went through the pointer, not a copy).

Heap version — same idea, longer lifetime:

```c
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *q = malloc(sizeof(int));
    if (!q) return 1;
    *q = 123;
    printf("heap int=%d\n", *q);
    free(q);
    return 0;
}
```

What this does: asks the [heap](../../glossary/terms.md#heap) for 4 bytes, writes through the returned pointer, then gives the bytes back.

| Lines | Code | Why it exists |
|---|---|---|
| 5 | `malloc(sizeof(int))` | request 4 bytes; returns address or `NULL`; OS-why: `kalloc` does this for kernels |
| 6 | `if (!q) return 1;` | always check for out-of-memory; real code, not decoration |
| 7–8 | `*q = 123;` + print | same dereference trick, but memory lives until `free` (not tied to this function) |
| 9 | `free(q);` | return the bytes; forgetting this = leak; freeing twice = heap corruption |

Change X → Y: change `sizeof(int)` to `sizeof(int)*3` and loop `q[0]=1; q[1]=2; q[2]=3;`. Verify: `make run` prints all three (proves `malloc` gives contiguous slots, `q[i]` is sugar for `*(q+i)`).

## Build It (QEMU-adjacent — hosted sim is enough here)

This lesson is primer, so "bare-metal" means: notice what disappears. Hosted C gave you `printf`, `malloc`, `free`. In `kernel.*` (freestanding, `-ffreestanding`) none of those exist until you write a UART print + a page allocator. Try compiling the same file freestanding to feel the gap:

```bash
cc -ffreestanding -nostdlib -c main.c -o /tmp/main.o && echo "compiled, but nothing to print with yet"
```

What this does: compiles without assuming hosted libc startup or `printf` linkage; proves the syntax is fine but the runtime is gone.

| Lines | Code | Why it exists |
|---|---|---|
| `cc -ffreestanding` | freestanding mode | tells gcc "no `main` startup, no libc" — kernel mode |
| `-nostdlib -c` | compile only, no link | we only check syntax; linking needs a linker script (P-02-06) |
| `echo ...` | confirmation | if this prints, your pointers are valid C even without an OS under them |

Change X → Y: remove `-ffreestanding` → links fine on host. Verify: output still `compiled...` but now you know the flag is what stripped the hosted assumptions.

## Use It (Linux)

Watch real addresses move on your machine:

```bash
./build/sim && echo "exit=$?"
cat /proc/self/maps | head -5
```

What this does: runs your simulator, prints its exit code, then shows your shell's own memory map (stack, heap, code regions).

| Lines | Code | Why it exists |
|---|---|---|
| `./build/sim` | run the sim | proves pointers work on real Linux virtual memory |
| `echo "exit=$?"` | show exit status | `return 0` in `main` becomes `$?`; OS-why: parent reads this via `wait()` (next OS lesson) |
| `cat /proc/self/maps` | inspect mappings | `cat`'s own stack/heap/code layout — the same boxes from the 60s diagram, live |

Change X → Y: run `cat /proc/$$/maps | head -5` (your shell instead of `cat`). Verify: addresses differ per process — each process has its own virtual [address](../../glossary/terms.md#address) space (OSTEP Ch.13 preview).

## Ship It

Artifact: `outputs/runbook-pointers.md` — one-page "pointer triage" checklist (null? freed? off-by-sizeof?). Use it whenever GDB prints a weird address in later phases.

Reuse in 2 lines: open the runbook next to GDB; when a crash prints an address, walk the 3 checks (NULL → freed → `p+1` stride).

## Exercises

1. Easy — print `sizeof(int)`, `sizeof(char)`, `sizeof(void*)` on your machine. Explain why `p+1` differs for `char*` vs `int*`.
2. Medium — allocate 5 ints, fill `q[i]=i*10`, print via both `q[i]` and `*(q+i)`. Prove they match.
3. Hard — implement `void swap(int *a, int *b)` using only `*a`/`*b` + temp. Call from `main`, show `x,y` swapped (preview of PCB field updates).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| pointer | variable holding a memory address | ../../glossary/terms.md#pointer |
| address | numbered byte slot; `&x` takes it | ../../glossary/terms.md#address |
| dereference | `*p` = go to the address and touch what's there | ../../glossary/terms.md#pointer |
| heap | long-lived bytes via malloc/free | ../../glossary/terms.md#heap |
| freestanding | C with no libc; you provide print/alloc | ../../glossary/terms.md#freestanding |

## Further Reading

- OSTEP Ch.13 — address spaces, why every process gets its own mailboxes (1 diagram to stare at).
- `man 3 malloc` — exact contract of what we called (return, NULL, free pairing).

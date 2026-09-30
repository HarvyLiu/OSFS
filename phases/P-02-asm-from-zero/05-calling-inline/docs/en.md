# C Calling ASM, Inline ASM — Constraints Are the Contract

> Template says what. Outputs/inputs say where. Clobbers confess the rest.

**Type:** Build
**Languages:** ASM, C
**Prerequisites:** 04-interrupts-idt
**College ref:** OSTEP Ch.6 (traps need exact register control), MIT 6.1810 inline-asm helpers (`riscv.h` style), xv6 file kernel/riscv.h (read/write-satp shapes yours mirror)
**Time:** ~75 minutes

## Learning Objectives
- Trace `__asm__ ("addl %1, %0" : "+r"(x) : "r"(y))` slot by slot: who fills `%0`, who reads `%1`
- Implement `rdtsc`, `cpuid`-vendor, and constraint-based add in header-inline helpers that run on host
- Explain `"=r"` vs `"+r"` vs `"a"`/`"Nd"`, `volatile`, and `"memory"`/`"cc"` clobbers
- Connect these to Tooling 02's `outb`/`inb` (same shapes, privileged ports) and to timing preemption cost

## Concept in 60s

![inline asm anatomy](../figures/inline-asm.svg)

<!-- source: ../figures/inline-asm.excalidraw — open in excalidraw.com to redraw -->

Extended ASM is a function call to the compiler's allocator: `__asm__ (TEMPLATE : OUTPUTS : INPUTS : CLOBBERS)`. `%0`, `%1`… name the operands *in order* (outputs first, then inputs). `"=r"(x)` = write-only, any [register](../../glossary/terms.md#register); `"+r"(x)` = read-write; `"a"` = must be `%eax`; `"Nd"` = imm8-or-`%dx` (port shapes). `volatile` = never delete/reorder (hardware pokes). Clobbers confess side effects: `"cc"` (flags changed), `"memory"` (RAM touched beyond listed outputs — blocks caching across the ASM). Get one letter wrong and the compiler "optimizes" your hardware access into nothing.

## Simulate It (host C — the contract in plain C first)

The allocator's job, modeled: outputs written, inputs read, clobbers invalidated.

```c
#include <stdio.h>

// add_asm modeled: out starts as x, plus y.
static int model_add(int x, int y) { int out = x; out = out + y; return out; }

int main(void) {
    printf("model_add(40,2)=%d\n", model_add(40, 2));
    return model_add(40, 2) != 42;
}
```

What this does: states the dataflow (`out=x; out+=y`) the real template below asks the compiler to wire — read this, then the `%0/%1` line reads itself.

| Lines | Code | Why it exists |
|---|---|---|
| 4 | `out = x` then `+= y` | mirrors `"+r"(x)` (seeded output) + `"r"(y)` (pure input): two roles, two constraints |

Change X → Y: change `40` to `-5`, expect `-3`. Verify: dataflow, not constants (same move as every prior sim).

## Build It (real helpers, run on host — all userspace-legal)

One header, `code/asmops.h` — `static inline` so every lesson can include it freestanding-friendly (no libc inside):

```c
// asmops.h -- rdtsc, add-via-template, cpuid vendor. Lesson docs/en.md.
#ifndef OSFS_ASMOPS_H
#define OSFS_ASMOPS_H

#include <stdint.h>

static inline int add_asm(int x, int y) {
    __asm__ ("addl %1, %0" : "+r" (x) : "r" (y) : "cc");
    return x;
}

static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

static inline void cpuid_vendor(char out[13]) {
    uint32_t b, c, d;
    uint32_t a = 0;
    __asm__ volatile ("cpuid"
                      : "=b" (b), "=d" (d), "=c" (c)
                      : "a" (a));
    __builtin_memcpy(out + 0, &b, 4);
    __builtin_memcpy(out + 4, &d, 4);
    __builtin_memcpy(out + 8, &c, 4);
    out[12] = 0;
}

#endif
```

What this does: three callable instructions — arithmetic through the allocator, a cycle counter, and a vendor query — each showing one constraint family.

| Lines | Code | Why it exists |
|---|---|---|
| 8 | `"addl %1, %0"` | AT&T `src,dst`: `%0`=first listed (`x`), `%1`=second (`y`) — numbering follows declaration order, outputs-then-inputs |
| 8 | `"+r"(x)` | read-write, any register: seeds `%0` with x, writes result back to x (C sees the update — that's the `+`) |
| 8 | `"r"(y)` + `"cc"` | pure input anywhere + flags-clobber confession (`add` sets flags; without it, a nearby `if` could miscompile) |
| 13 | `volatile` + `"=a"/"=d"` | `rdtsc` *must* execute (timing!) and lands fixed in `%eax`/`%edx` — pinned constraints, unmovable statement |
| 14 | shift-OR combine | `hi:lo` → 64-bit stamp (TSC ticks since reset — brickyard clock for Exercise 3's preemption costing) |
| 19–22 | `cpuid` | leaf 0 in `"a"`, vendor pieces out of `%ebx/%edx/%ecx` *in that order* (`b,d,c` — Intel's quirky spread, memorized once here) |
| 23–25 | `__builtin_memcpy` | byte-copy without libc (freestanding-safe — plain `memcpy` would assume hosted) |

Change X → Y: drop `volatile` from `rdtsc`, rebuild `-O2`, call it twice around an empty loop. Verify: deltas collapse toward 0 (compiler hoisted/merged the "pure" reads — the demo that makes `volatile` unforgettable).

Caller (`code/main.c`):

```c
#include <stdio.h>
#include "asmops.h"

int main(void) {
    int s = add_asm(40, 2);
    uint64_t t0 = rdtsc();
    volatile int sink = 0;
    for (int i = 0; i < 1000; i++) sink += i;
    uint64_t t1 = rdtsc();
    char vendor[13];
    cpuid_vendor(vendor);
    printf("add=%d dt=%llu cycles sink=%d vendor=%s\n", s,
           (unsigned long long)(t1 - t0), sink, vendor);
    return s != 42 || t1 <= t0;
}
```

What this does: proves arithmetic, measures 1000 adds in cycles, names your silicon — allocator, counter, query in one run.

| Lines | Code | Why it exists |
|---|---|---|
| 6–8 | `volatile sink` loop | `volatile` forces all 1000 adds to really execute (else `-O2` folds to `499500` with ~0 cycles — try it, it's Exercise 1) |
| 9 | `t1 - t0` | delta cycles: machine time for the loop (varies run to run — ranges, not constants, are the expectation) |
| 13 | `t1 <= t0` fails | TSC must advance (a frozen counter = broken virtualization or ancient silicon — the check earns its line) |

Change X → Y: remove `volatile` from `sink`. Verify: `dt` craters (proves the counter is honest and the compiler is clever — same lesson as `volatile` above, from the other side).

 privileged shapes (reference — Tooling 02's pair, decoded with today's grammar):

```c
// outb %al, %dx  ==  "outb %0, %1" : : "a"(val), "Nd"(port)
// inb %dx, %al  ==  "inb %1, %0"  : "=a"(ret) : "Nd"(port)
```

What this does: translates the two hardware pokes you already use into constraint sentences — `"a"` pins `%al`, `"Nd"` allows imm8-or-`%dx`, outputs-first numbering explains the `%0/%1` flip between them.

## Use It (Linux)

See your compiler thinking in constraints:

```bash
gcc -O2 -S code/main.c -o /tmp/main.s && grep -E "rdtsc|cpuid|addl" /tmp/main.s | head -6
grep -rn "asm volatile" /usr/include/x86_64-linux-gnu/asm/ 2>/dev/null | head -3 || echo "(kernel headers absent: containers/WSL may lack them; the repo examples stand alone)"
```

What this does: finds your three instructions in generated assembly, then peeks at system headers' own `asm volatile` usage (same dialect, production seasoning).

| Lines | Code | Why it exists |
|---|---|---|
| `grep rdtsc\|cpuid` | self-recognition | your helpers survived `-O2` verbatim (volatile + pinned constraints vs optimizer — witnessed) |
| system headers grep | dialect proof | identical `__asm__ volatile (...)` shapes outside this course (fallback echo keeps Docker-slim honest) |

Change X → Y: rebuild the `-S` with `-O0`. Verify: identical `rdtsc`/`cpuid` lines amid far more `mov` spill (volatile instructions are optimization-proof; the C around them isn't).

## Ship It

Artifact: `outputs/constraints-card.md` — template/output/input/clobber anatomy, letter table (`r/a/b/c/d/S/D/Nd/I`), `volatile` rule (hardware + timing = always), `"memory"` rule (RAM touched = confess), plus the `outb`/`inb` pair decoded. Tape beside the ASM cheatsheet; together they read any `riscv.h`-shaped header.

## Exercises

1. Easy — remove `volatile` from `sink`, record `dt` before/after at `-O2` (expect collapse → constant-fold proof).
2. Medium — write `mul_asm` (`imull %1,%0`, `"+r"`, `"cc"`), verify against C across negatives (flags + signedness, one constraint reused).
3. Hard — time 100k `add_asm` vs plain `+` calls in a loop (TSC deltas, 5 runs, report range+median): same codegen? (Usually identical — the lesson that inline ASM buys *access*, not speed, except for instructions C can't spell.)

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| register | constraint letters pick them (`r` any, `a` eax, `Nd` port) | ../../glossary/terms.md#register |
| address | ports are addresses too (`outb` writes one); `lea` computes them | ../../glossary/terms.md#address |
| syscall | `cpuid`/`rdtsc` are *instructions*, not traps — no kernel crossing (contrast!) | ../../glossary/terms.md#syscall |

## Further Reading

- GCC manual, Extended Asm — the constraint + clobber reference (read the `+`/`=`/`&` modifiers once).
- xv6-style `riscv.h`/`x86.h` headers — same shapes, other ISA (dialect tourism, highly recommended).
- Intel SDM Vol.2 `RDTSC`/`CPUID` — exact register contracts we encoded.

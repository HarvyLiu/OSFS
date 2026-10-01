# Registers, mov, add — Your First AT&T Trace

> The CPU only trusts registers. Memory is a suburb; registers are home.

**Type:** Build
**Languages:** ASM, C
**Prerequisites:** 01-bits-binary-hex
**College ref:** OSTEP Ch.4 (registers are the process), MIT 6.1810 Lec 3 (calling convention), xv6 file kernel/swtch.S (where this leads)
**Time:** ~60 minutes

## Learning Objectives
- Trace `%edi/%esi → %eax` through `movl/addl/ret` using GDB `info registers`
- Implement a C↔ASM call (`add2`) in GAS AT&T and read args/return correctly
- Explain `$` vs `%`, suffixes `b/w/l/q`, and `src → dst` order without hesitating
- Connect the return register to every `fork()` return-value trick later

## Concept in 60s

![registers map](../figures/registers-map.svg)

<!-- source: ../figures/registers-map.excalidraw — open in excalidraw.com to redraw -->

You call `add2(40, 2)` and the numbers land in [registers](../../../../glossary/terms.md#register) before any stack is touched: first int in `%edi`, second in `%esi`, return in `%eax` (32-bit slices of `%rdi/%rsi/%rax`). You read `movl $1, %eax` as "copy immediate 1 into eax": `$`=number, `%`=register, `l`=32 bits, order is source-then-destination. `addl %esi, %eax` adds esi *into* eax. `ret` jumps back. That dataflow is the whole trick — the rest is vocabulary.

## Simulate It (host — C model of registers, no ASM yet)

Before touching ASM, prove you know the dataflow in C:

```c
#include <stdio.h>

int main(void) {
    int edi = 40, esi = 2;   // pretend: incoming args
    int eax = edi;           // movl %edi, %eax
    eax = eax + esi;         // addl %esi, %eax
    printf("40+2=%d\n", eax);
    return eax != 42;
}
```

What this does: mimics exactly what the 3-instruction ASM does, so when GDB shows `%eax=42` later you already believe it.

| Lines | Code | Why it exists |
|---|---|---|
| 4 | `edi/esi` vars | stand-ins for incoming [registers](../../../../glossary/terms.md#register); System V puts arg1/arg2 here |
| 5 | `eax = edi` | the `movl`: copy, don't move (source keeps its value — `mov` is a bad name, it's `copy`) |
| 6 | `eax + esi` | the `addl`: result accumulates in destination (`%eax`) |
| 8 | `return eax != 42` | exit 0 iff math right — machine-checkable proof, same as `make test` |

Change X → Y: change `40` to `100`. Verify: output `100+2=102`, exit still 0 (proves the flow, not the constants).

## Build It (C calls AT&T ASM — Linux/WSL/Docker-only for execution)

Files in `code/`: `add.s`, `main.c`, `Makefile`. Assemble + link with plain `cc` — GAS speaks AT&T natively. Note: this lesson needs the System V ABI (Linux: args in `%edi/%esi`). It *assembles* anywhere GAS runs (including Windows gcc), but *runs correctly* only on System V — Windows x64 passes args in `%ecx/%edx` instead, so the same bytes compute the wrong sum there. Patriots of portability: that mismatch is exactly why calling conventions are standardized per OS.

The whole ASM file:

```asm
.text
.globl add2
add2:
    movl %edi, %eax
    addl %esi, %eax
    ret
```

What this does: takes two ints from their registers, returns the sum in `%eax` — a callable function, not a fragment.

| Lines | Code | Why it exists |
|---|---|---|
| 1 | `.text` | code section (not data); CPU fetches here |
| 2 | `.globl add2` | export symbol so `call add2` links (ELF builds may add optional `.type add2, @function`; omitted here so Windows GAS also assembles) |
| 5 | `movl %edi, %eax` | copy arg1 into return reg; `l` = 32-bit ints; AT&T order `src, dst` |
| 6 | `addl %esi, %eax` | `eax += esi`; destination holds the running result |
| 7 | `ret` | pop return [address](../../../../glossary/terms.md#address) into `%rip` — back to C |

Change X → Y: swap to `addl %edi, %esi` + `movl %esi, %eax` (same sum, extra step). Verify: `make run` still prints 42 (proves destination choice is convention + efficiency, not magic).

| AT&T (we write) | Intel (you may read) | Note |
|---|---|---|
| `movl %edi, %eax` | `mov eax, edi` | AT&T `src,dst`; Intel `dst,src`. Ours has `%`/`l`. |
| `addl %esi, %eax` | `add eax, esi` | same flip; result always lands in `eax` here |
| `movl $1, %eax` | `mov eax, 1` | `$` = immediate; Intel uses bare number |

Caller (hosted C):

```c
#include <stdio.h>
int add2(int a, int b);

int main(void) {
    int r = add2(40, 2);
    printf("add2(40,2)=%d\n", r);
    return r != 42;
}
```

What this does: declares the ASM symbol (no header needed for one function), calls it like any C function, exits 0 on success.

| Lines | Code | Why it exists |
|---|---|---|
| 2 | `int add2(int a, int b);` | prototype: tells C "args in edi/esi, result in eax" without seeing the ASM |
| 5 | `add2(40, 2)` | compiler emits arg setup + `call add2`; your ASM runs between call and return |
| 7 | `return r != 42` | `!=` yields 0/1; exit 0 = GDB will agree `%eax` was 42 |

Change X → Y: call `add2(-5, 5)`. Verify: prints 0, exit 0 (proves signed ints ride the same path — two's complement from P-00).

Trace it under GDB (the skill every later crash needs):

```bash
make run
gdb -batch -ex 'break add2' -ex run -ex 'info registers eax edi esi' -ex continue ./build/regs
```

What this does: builds, stops *at* your ASM, prints the three live registers, then finishes — the exact loop for all future ASM debugging.

| Lines | Code | Why it exists |
|---|---|---|
| `break add2` | stop at symbol | works because `.globl` exported it; no addresses to memorize |
| `info registers eax edi esi` | show trio | expect `edi=40 esi=2 eax=40` at entry (after `movl`, `eax` becomes 40; after `addl`, 42 — step with `si` to watch) |
| `continue` + batch | finish non-interactively | `-batch` = scriptable, CI-safe; drop it for interactive `layout asm` sessions |

Change X → Y: add `-ex 'si' -ex 'info registers eax'` after the first info. Verify: `eax` steps 40 → 42 — you just watched one instruction execute (the core GDB move).

## Use It (Linux)

See the same convention in real binaries:

```bash
objdump -d build/regs | grep -A6 "<add2>:"
nm build/regs | grep add2
```

What this does: disassembles *your* function as Linux sees it (AT&T by default) and proves the symbol made it into the binary.

| Lines | Code | Why it exists |
|---|---|---|
| `objdump -d ... add2` | disassembly | `objdump` prints AT&T unless `--disassembler-options=intel` — why we standardized on AT&T |
| `nm ... add2` | symbol table | `T add2` = text-section global; lowercase `t` would mean hidden (link would fail) |

Change X → Y: rerun objdump with `--disassembler-options=intel`. Verify: `mov eax,edi` appears — same CPU, flipped printing (now you can read both dialects without fear).

## Ship It

Artifact: `outputs/asm-cheatsheet.md` — `$`/`%`/suffixes/order + 15 instructions + GDB 5-liner. Keep open for the entire P-02 track; by 02/03 you'll read `swtch.S`-shaped code cold.

## Exercises

1. Easy — add `sub2` (`eaxe bis`): `movl %edi, %eax; subl %esi, %eax`. Call `sub2(10,4)`, expect 6.
2. Medium — in GDB, `break *add2+2` (second instruction), show `eax` is already 40 before the add. Explain why (proves `movl` ran).
3. Hard — write `mul2` with `imull %esi, %eax`. Predict flags, verify `add2(1000000,1000000)` vs `mul2(100000,100)` overflow differently (preview of `mul` vs `add` semantics).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| register | CPU-home storage (`%eax/%edi/%esi`); fastest, smallest | [register](../../../../glossary/terms.md#register) |
| address | where code lives (`nm`/`objdump` print these in hex) | [address](../../../../glossary/terms.md#address) |
| stack | where `call` pushes return addresses (next lesson) | [stack](../../../../glossary/terms.md#stack) |

## Further Reading

- `man 1 gdb` `info registers`, `si`, `layout asm` — the 3 verbs used here.
- MIT 6.1810 calling-convention notes — same `%rdi/%rsi/%rax` family, 64-bit version.
- xv6 `kernel/swtch.S` (skim only) — 15 lines you'll fully read after P-02/02.

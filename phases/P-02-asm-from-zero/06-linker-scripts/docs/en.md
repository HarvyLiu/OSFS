# Linker Scripts — Where the Kernel Lives

> Compiler makes bricks. Linker builds the house — at addresses you choose.

**Type:** Learn
**Languages:** ASM, C
**Prerequisites:** 05-calling-inline
**College ref:** OSTEP Ch.13 (address spaces need layout), MIT 6.1810 `kernel.ld` (the real script), xv6 file kernel/kernel.ld (entry + sections)
**Time:** ~60 minutes

## Learning Objectives
- Trace objects → sections → ELF layout using `objdump -h` and `nm` on your own binary
- Implement a custom section variable and read its address back against `.text`/`.data`
- Explain `ENTRY`, location counter (`.`), `ALIGN`, and why `.multiboot` must come first
- Connect host layout (loader picks addresses) to kernel layout (script *is* the loader's orders)

## Concept in 60s

![link layout](../figures/link-layout.svg)

<!-- source: ../figures/link-layout.excalidraw — open in excalidraw.com to redraw -->

Each `.o` carries *sections* (`.text` code, `.rodata` constants, `.data` initialized globals, `.bss` zeroed globals). The linker script's `SECTIONS` block concatenates them into one image at chosen addresses: `. = 1M` sets the location counter (this [address](../../glossary/terms.md#address) onward), `*(.multiboot)` pulls every multiboot header first, `ALIGN(4K)` pads to page boundaries (Memory phases will bless you for this). `ENTRY(_start)` stamps where the first jump lands. On host Linux, the default script + loader do this invisibly; kernels ship their own because *they* are the bottom.

## Simulate It (host — sections you can touch, no QEMU)

Full program: `code/sections.c`. One variable per section class, printed by address.

```c
#include <stdio.h>

int init_global = 41;
int zero_global;
const int ro_const = 7;
int mysec_var __attribute__((section(".mysec"))) = 99;

int main(void) {
    static int stat_init = 5;
    static int stat_zero;
    int local = 1;
    printf("text(main)=%p ro=%p data=%p bss=%p mysec=%p stack~%p\n",
           (void *)main, (const void *)&ro_const, (void *)&init_global,
           (void *)&zero_global, (void *)&mysec_var, (void *)&local);
    printf("values: %d %d %d %d %d %d\n", init_global, zero_global,
           ro_const, mysec_var, stat_init, stat_zero + local);
    (void)stat_init;
    return init_global != 41 || ro_const != 7 || mysec_var != 99;
}
```

What this does: plants one resident in each section class, then prints addresses (layout proof) + values (sanity gate) — `objdump`'s claims made tangible.

| Lines | Code | Why it exists |
|---|---|---|
| 3–5 | three globals | initialized → `.data`, zero → `.bss`, `const` → `.rodata`: storage class *is* section destiny |
| 6 | `section(".mysec")` | custom section: linker collects all `.mysec` inputs together (kernels isolate multiboot/init code this way) |
| 8–9 | statics + local | `static` = global lifetime, function scope (`.data`/`.bss` again); `local` = [stack](../../glossary/terms.md#stack) (far away in address — compare the hex!) |
| 11–13 | address print | `%p` parade: text lowest (code), then ro/data/bss clump, stack far high (ASLR jitters run to run — order stable, digits not) |

Change X → Y: change `mysec_var` init `99` → `100`. Verify: values line shows `100`, address unchanged (proves content vs placement are independent axes — compiler owns one, linker the other).

## Build It (read your own binary + dissect the kernel script)

```bash
make run
objdump -h build/sections | grep -E "text|data|bss|rodata|mysec"
nm build/sections | grep -E " (main|init_global|zero_global|mysec_var)$"
```

What this does: runs the sim, lists section addresses/sizes, and resolves your symbols to sections — the script's output, audited.

| Lines | Code | Why it exists |
|---|---|---|
| `objdump -h` | section table | `VMA` = runtime [address](../../glossary/terms.md#address), `Size` = footprint; spot `.mysec` sitting apart (your custom island, listed) |
| `nm ... grep` | symbol → section | `T main` (text), `D init_global` (data), `B zero_global` (bss), `R ro_const` (rodata): the letter *is* the section |

Change X → Y: `objdump -h` on `/bin/ls` instead. Verify: same section names, wildly different VMAs (host loader relocates; kernels can't rely on that — hence scripts).

Tooling 02's script, decoded line by line (the one that boots):

```ld
ENTRY(_start)
SECTIONS {
    . = 1M;
    .text : { *(.multiboot) *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : { *(.bss) }
}
```

What this does: pins code at 1 MiB with the magic header first, then constants, data, zeroed tail — a complete kernel layout in 8 lines.

| Lines | Code | Why it exists |
|---|---|---|
| `ENTRY(_start)` | first jump | must match `.globl _start` (P-02/01's handshake, now load-bearing: QEMU jumps here) |
| `. = 1M` | location counter | 0x100000: above BIOS/real-mode area, low enough for 32-bit (the classic PC contract) |
| `*(.multiboot)` first | header placement | QEMU scans the first 8 KiB for `0x1BADB002`; `.text` first would bury it → `not a bootable kernel` |
| rodata/data/bss order | grouping | keeps read-only together (Memory phases mark it non-writable per-page — layout enables protection) |

Change X → Y: swap `.rodata` and `.data` lines, rebuild Tooling 02 (`make -C` there). Verify: still boots (order of *these two* is convention, not contract — but moving `.multiboot` breaks everything; try that in a scratch copy to feel the difference).

## Use It (Linux)

Layout forensics on anything:

```bash
file build/sections | cut -c1-100
readelf -l build/sections 2>/dev/null | grep -A1 LOAD | head -8 || objdump -p build/sections | grep -A2 "LOAD" | head -8
```

What this does: identifies the binary kind, then shows program headers (what the loader `mmap`s where — the script's runtime shadow).

| Lines | Code | Why it exists |
|---|---|---|
| `file` | kind stamp | `ELF 64-bit ... dynamically linked` (host) vs Tooling 02's `statically linked` (kernel: no interpreter, no shared libs — self-contained by necessity) |
| `readelf -l` | LOAD segments | each = one `mmap`: file bytes → virtual addresses with RWE flags (linker groups sections into segments; loader enforces) |

Change X → Y: `readelf -l` on Tooling 02's `kernel.elf` (Linux box). Verify: single low `LOAD` near `0x100000` (your script's `. = 1M`, visible in production form).

## Ship It

Artifact: `outputs/linker-card.md` — `ENTRY`/dot/`ALIGN`/section-order checklist + `objdump -h`/`nm`/`readelf -l` verbs + the 8-line kernel script as starter. Bring it to every Memory-phase page-table session (alignment starts here).

## Exercises

1. Easy — add a second `.mysec` variable in another file, link both, show `nm` places them adjacently (sections collect across objects — the mechanism init arrays use).
2. Medium — add `ALIGN(16)` between two sections in a scratch script copy for Tooling 02's kernel, rebuild, compare `objdump -h` VMAs (padding appears — measure it in bytes).
3. Hard — write a `PROVIDE(heap_start = .)` + `PROVIDE(heap_end = .)` pair around `.bss` end, print them from kernel serial (this *is* how `kalloc` learns its arena — implemented for real in Memory II).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| address | linker assigns final ones (host: suggestion; kernel script: law) | ../../glossary/terms.md#address |
| stack | not in the image (runtime-erected by `boot.s` — the one section money can't buy) | ../../glossary/terms.md#stack |
| freestanding | no default script/services: you specify layout + entry + libs(=none) | ../../glossary/terms.md#freestanding |

## Further Reading

- `man 1 ld` (`ENTRY`, `SECTIONS`, `.`, `ALIGN`, `PROVIDE`) — the 5 verbs used here.
- xv6 `kernel/kernel.ld` — 15 lines; every one now parses.
- OSDev Bare Bones linker chapter — `*(.multiboot)` ordering war stories.

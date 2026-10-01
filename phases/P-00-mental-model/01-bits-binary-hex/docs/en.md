# Bits, Binary, Hex — Reading Memory Like the CPU Does

> Hex is just binary with better PR. Learn to read both and dumps stop being scary.

**Type:** Learn
**Languages:** C
**Prerequisites:** None
**College ref:** OSTEP Ch.1 (addresses are numbers), MIT 6.1810 Appendix (C/hex refresher), xv6 file kernel/memlayout.h (addresses you'll read)
**Time:** ~45 minutes

## Learning Objectives
- Trace bits → hex → bytes using a nibble table from memory
- Implement shifts, masks, and `%x` dumps in hosted C
- Explain why addresses print in hex (`0x...`) and sizes in decimal
- Connect nibbles to real dumps (`objdump`, `xxd`, GDB `x/x`)

## Concept in 60s

![bits map](../figures/bits-map.svg)

<!-- source: ../figures/bits-map.excalidraw — open in excalidraw.com to redraw -->

One hex digit = 4 bits = one nibble. `0xA` = `1010` = 10. Two hex digits = one byte (`0x00`–`0xFF`). Addresses are hex because 64 bits in binary is unreadable but in hex is 16 chars (`0x7fff...`). Bit ops: `<<` = ×2ⁿ, `>>` = ÷2ⁿ, `&` = mask (keep these bits), `|` = set, `^` = flip. An [address](../../../../glossary/terms.md#address) is just a big number printed in hex.

## Simulate It (host, no QEMU)

Full program: `code/main.c` — prints the same value three ways plus shifts.

```c
#include <stdio.h>

int main(void) {
    unsigned char b = 0xA6;
    printf("hex=%#x dec=%u bits: ", b, b);
    for (int i = 7; i >= 0; i--) printf("%d", (b >> i) & 1);
    printf("\nmask low-nibble: %#x\n", b & 0x0F);
    printf("1<<4 = %d, 256>>3 = %d\n", 1 << 4, 256 >> 3);
    return 0;
}
```

What this does: takes byte `0xA6`, shows hex/decimal/binary views, isolates the low nibble with a mask, and proves shifts are multiply/divide by powers of two.

| Lines | Code | Why it exists |
|---|---|---|
| 4 | `unsigned char b = 0xA6;` | `unsigned` = 0–255 (no sign bit confusion); `0x` prefix = hex literal (`A`=10, `6`=6) |
| 5 | `%#x %u` | `%#x` = hex with `0x`, `%u` = unsigned decimal — same byte, two views (like `%c`/`%d` for chars) |
| 6 | `(b >> i) & 1` | shift bit `i` to position 0, mask all but it — prints MSB→LSB (`i` from 7 down) |
| 7 | `b & 0x0F` | `0x0F` = `00001111`: keeps low 4 bits, kills high — how page flags/permission bits get read later |
| 8 | `1<<4`, `256>>3` | `<<4` = ×16 → 16; `>>3` = ÷8 → 32; OS-why: alignment (`addr & ~0xFFF`) and page math use exactly this |

Change X → Y: change `0xA6` to `0x0F`. Verify: `make run` → bits `00001111`, mask prints `0xf` (proves the loop reads the value, not a fixed string).

## Build It (compile + dump — see your bytes)

```bash
make run
xxd build/bits | head -3
objdump -d build/bits | grep -A3 "<main>:" | head -8
```

What this does: runs the sim, then shows its raw file bytes and disassembled `main` — the same binary/hex views from Concept, live.

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | compile + run | `-Wall -Werror` still on; output ends with mask + shift lines |
| `xxd ... head -3` | hex dump | left = file offsets (hex), middle = bytes (hex), right = ASCII — read middle column as nibbles |
| `objdump -d ... main` | disassembly | your C as machine code; addresses in hex (`0x...`), opcodes in hex — why hex won |

Change X → Y: pipe `xxd` into `grep OSFS` vs your bits binary. Verify: text binary contains strings, bits binary doesn't — proves `xxd` shows what's really inside, not what ran.

## Use It (Linux)

Addresses are hex in the wild:

```bash
printf '%p %x %d\n' 12345 12345 12345
cat /proc/self/maps | head -3
gdb -batch -ex 'p/x 255' -ex 'p/t 255'
```

What this does: prints one number three ways, shows real process addresses (hex), and uses GDB as a hex/binary calculator.

| Lines | Code | Why it exists |
|---|---|---|
| `printf '%p %x %d'` | shell triple-view | `%p` = pointer-style hex, `%x` = plain hex, `%d` = decimal — same 12345 three costumes |
| `/proc/self/maps` | live addresses | every range is hex (`7f...-7f...`); you now read the high digits as "which region" |
| `gdb p/x p/t` | calculator | `p/x` = hex, `p/t` = binary (`t`=two); faster than mental math during debugging |

Change X → Y: `p/x 4096` → `0x1000`. Verify: 4096 = one page (4 KiB) — the number behind every `1M`/`0x100000` in linker scripts (Tooling 02).

## Ship It

Artifact: `outputs/hex-card.md` — nibble table + `<<`/`>>`/`&` one-liners + GDB verbs. Keep open during every `objdump`/GDB session from here on.

## Exercises

1. Easy — print 0–15 as dec/hex/4-bit binary (hand-check `10`=`0xa`=`1010`).
2. Medium — given `0x3F8`, print `COM1+5` in hex and decimal (Tooling 02 address, now readable).
3. Hard — extract bits 12–15 of `0xA6F3` via `(x >> 12) & 0xF`; explain which nibble you isolated (preview of page-table index math).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| address | byte number, printed hex by convention | [address](../../../../glossary/terms.md#address) |
| register | bit bucket (`%eax`); shifts/masks live here | [register](../../../../glossary/terms.md#register) |

## Further Reading

- `man 1 xxd` — dump flags you'll reuse on kernels (`-b` for binary, `-l` for length).
- OSTEP Ch.13 — addresses as numbers; revisit after this lesson and the diagram clicks.

# lea vs mov, Addressing Modes — Math That Never Touches Memory

> mov fetches. lea calculates. The brackets decide everything.

**Type:** Build
**Languages:** ASM, C
**Prerequisites:** 02-stack-call-ret
**College ref:** OSTEP Ch.13 (address arithmetic = paging math preview), MIT 6.1810 pointer/addressing notes, xv6 file kernel/vm.c (page-index arithmetic like this)
**Time:** ~60 minutes

## Learning Objectives
- Trace `leal (%rdi,%rdi,2),%eax` as ×3 without a multiply or memory touch
- Implement scaled-index loads `(%rdi,%rsi,4)` and struct-field `8(%rdi)` in AT&T ASM
- Explain `mov` (dereference) vs `lea` (address math) using one changed character
- Connect scaled indexing to `arr[i]`, PCB tables, and page-table walks

## Concept in 60s

![addressing modes](../figures/addressing-map.svg)

<!-- source: ../figures/addressing-map.excalidraw — open in excalidraw.com to redraw -->

AT&T memory syntax: `offset(base,index,scale)` = `base + index*scale + offset`, where scale ∈ {1,2,4,8}. `movl 8(%rdi), %eax` *loads* the 4 bytes at that [address](../../../../glossary/terms.md#address). `leal 8(%rdi), %eax` *computes* the address itself into `%eax` — no memory touched. Compilers abuse `lea` for fast multiply-add (`x*3` = `x + x*2`). Struct fields are just offsets (`rsp` at +8 when `pid` at +0 — your PCB from P-01/05). Array indexing is just scale (`int` = ×4).

## Simulate It (host C — the math before the mnemonics)

```c
#include <stdio.h>

int main(void) {
    int arr[4] = {10, 20, 30, 40};
    int i = 2;
    int *base = arr;
    // (%rdi=%base, %rsi=%i, scale=4): addr = base + i*4
    int v = *(base + i);
    // lea trick: x*3 = x + x*2
    int x = 7, t = x + x * 2;
    // struct field = base + offset
    struct { int pid; int state; } p = {5, 2};
    int st = *(int *)((char *)&p + 8 - 4);
    printf("arr[2]=%d x3=%d state=%d\n", v, t, st);
    return v != 30 || t != 21 || st != 2;
}
```

What this does: performs the three addressing computations in C so the ASM lines below read as transliterations, not spells.

| Lines | Code | Why it exists |
|---|---|---|
| 8–9 | `*(base + i)` | C's `arr[i]`: pointer arithmetic strides by `sizeof(int)`=4 — the `,4` scale below |
| 11 | `x + x*2` | ×3 with shift-and-add shape; `lea` executes this in one instruction (no `imul`) |
| 13–14 | `(char*)&p + 4` | field at offset 4 = `base+offset` with index 0; byte-cast makes `+1` mean one byte (scale 1) |
| 16 | combined check | exit 0 iff all three shapes agree — one binary, three proofs |

Change X → Y: change `i = 2` to `i = 0`. Verify: prints `arr[0]=10` (proves the index, not the constant, selects — scale math is live).

## Build It (AT&T — Linux/WSL/Docker for execution, assembles anywhere)

Files: `addr.s` (three tiny functions), `main.c` (caller), same System V note as P-02/01–02.

```asm
.text
.globl scaled_get
scaled_get:
    movl (%rdi,%rsi,4), %eax
    ret

.globl mul3add
mul3add:
    leal (%rdi,%rdi,2), %eax
    ret

.globl field_state
field_state:
    movl 4(%rdi), %eax
    ret
```

What this does: loads `base[i]`, computes `x*3` without touching memory, loads a struct's second field — the three addressing shapes, callable from C.

| Lines | Code | Why it exists |
|---|---|---|
| 5 | `movl (%rdi,%rsi,4),%eax` | dereference `base + i*4`: `%rdi`=base, `%rsi`=i, `4`=[sizeof](../../../../glossary/terms.md#address) int; parens = go there |
| 10 | `leal (%rdi,%rdi,2),%eax` | *no* parens-in-spirit: compute `rdi + rdi*2` into `%eax`; `lea` never dereferences, even though the syntax looks like memory |
| 15 | `movl 4(%rdi),%eax` | field at offset 4 (state after pid): `base+4`, scale implied 1 |

Change X → Y: change scale `4` to `8` in `scaled_get`, call with an `int` array. Verify: garbage/half-values (proves scale must equal element size — `long` array would need 8; this is bug #1 in hand-rolled table walks).

| AT&T here | Intel elsewhere | Note |
|---|---|---|
| `movl (%rdi,%rsi,4), %eax` | `mov eax, [rdi+rsi*4]` | brackets vs parens; same base+index×scale |
| `leal (%rdi,%rdi,2), %eax` | `lea eax, [rdi+rdi*2]` | Intel `lea` also computes-only |
| `movl 4(%rdi), %eax` | `mov eax, [rdi+4]` | offset-first in AT&T, offset-last in Intel |

Caller:

```c
#include <stdio.h>
int scaled_get(int *b, int i);
int mul3add(int x);
int field_state(void *p);

struct rec { int pid; int state; };

int main(void) {
    int arr[4] = {10, 20, 30, 40};
    struct rec p = {5, 2};
    int a = scaled_get(arr, 2), m = mul3add(7), s = field_state(&p);
    printf("scaled=%d mul3=%d state=%d\n", a, m, s);
    return a != 30 || m != 21 || s != 2;
}
```

What this does: feeds the three functions known inputs; exit 0 iff addressing, math, and offsets all agree.

```bash
make run
gdb -batch -ex 'break scaled_get' -ex run -ex 'si' -ex 'info registers eax' -ex continue ./build/addr
```

What this does: runs the trio, then single-steps *one* load and shows `%eax` filling from memory — `mov` visibly fetches where `lea` would not.

| Lines | Code | Why it exists |
|---|---|---|
| `si` after break | step one instruction | lands past the `movl`: `%eax` now holds the loaded word (repeat on `mul3add` to contrast: no memory line in `strace`-style view, just math) |

Change X → Y: break on `mul3add`, `x/xw $rdi` before/after `si`. Verify: memory unchanged (proves `lea` touched no memory — the experiment that separates the two mnemonics forever).

## Use It (Linux)

Compilers think in these shapes constantly:

```bash
gcc -O2 -S -o /tmp/add2.s ../../P-01-c-from-zero/01-hello-c/code/main.c 2>/dev/null || gcc -O2 -S code/sim_addr.c -o /tmp/sim.s; grep -E "lea|mov" /tmp/sim.s | head -8
objdump -d build/addr | grep -A3 -E "scaled_get|mul3add"
```

What this does: shows `lea` in compiler output (it loves `lea` for arithmetic) plus your functions disassembled in canonical AT&T.

| Lines | Code | Why it exists |
|---|---|---|
| `gcc -O2 -S` | emit assembly | `-O2` leans on `lea`; `-O0` spells everything with `mov` (compare both — same C, different thrift) |
| `objdump` trio | your bytes back | `leal (%rdi,%rdi,2)` visible in the wild of your own binary |

Change X → Y: rebuild the dump with `-O0`. Verify: `lea` vanishes into plain `mov`/`add` chains (proves `lea`-as-math is an *optimization* of the C semantics, not different semantics).

`code/sim_addr.c` (fallback source if paths differ — same C as Simulate It, kept buildable standalone):

```c
#include <stdio.h>
int main(void) {
    int arr[4] = {10, 20, 30, 40};
    printf("arr[2]=%d x3=%d\n", arr[2], 7 + 7 * 2);
    return 0;
}
```

What this does: guarantees the `gcc -S` demo has a local file even if you moved lesson dirs around.

## Ship It

Artifact: `outputs/addressing-card.md` — the `offset(base,index,scale)` formula, scale table (char 1/short 2/int 4/long+pointer 8), `mov`-vs-`lea` one-liner, GDB `x/` verbs. Reuse in paging (page = base + index×4096 + offset — same shape, bigger numbers).

## Exercises

1. Easy — write `scaled_get_long(long *b, int i)` with scale 8. Prove with index 3 of `{1..5}`.
2. Medium — hand-assemble `x*5` as `lea` (`x + x*4`): which addressing form? Verify against C.
3. Hard — compute `&table[i]` for your PCB (`sizeof=40`) as `base + i*40` — no ASM, just math — then confirm with real pointer diffs (this *is* what the scheduler does per pick).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| register | base/index holders (`%rdi/%rsi`); result in `%eax` | [register](../../../../glossary/terms.md#register) |
| address | `offset(base,index,scale)` computes one; `mov` visits it | [address](../../../../glossary/terms.md#address) |
| heap | arrays/tables live here at runtime; scales walk them | [heap](../../../../glossary/terms.md#heap) |

## Further Reading

- `man 1 objdump` `-M intel` toggle — reread any dump in the other dialect.
- xv6 `kernel/vm.c:walk` (skim) — page-table indexing is scaled arithmetic wearing a robe.
- Intel/AMD manual Vol.1 addressing chapter (reference only — one page on SIB bytes explains *why* scales are 1/2/4/8).

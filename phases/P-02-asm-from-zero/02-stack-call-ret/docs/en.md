# Stack, call/ret, Frames — Where Functions Live

> call pushes the way home. The frame holds the locals. ret walks back.

**Type:** Build
**Languages:** ASM, C
**Prerequisites:** 01-registers-mov
**College ref:** OSTEP Ch.4 (stack per process), MIT 6.1810 calling convention (frames), xv6 file kernel/swtch.S (frames under switch)
**Time:** ~75 minutes

## Learning Objectives
- Trace `call`/`ret` + `push %rbp`/`mov %rsp,%rbp` frame setup using GDB `bt` and `x/` on the stack
- Implement a depth sampler proving the stack grows down, one frame per call
- Explain callee-saved `%rbp`/`%rbx` vs scratch regs and why `leave` mirrors the prologue
- Connect frames to context switch (each task's `%rsp` names its top frame) and to PCB trapframes

## Concept in 60s

![stack frames](../figures/stack-frames.svg)

<!-- source: ../figures/stack-frames.excalidraw — open in excalidraw.com to redraw -->

`call f` pushes the return [address](../../glossary/terms.md#address) and jumps. The classic prologue then pushes old `%rbp` and copies `%rsp` into `%rbp` — now `%rbp` anchors this frame while `%rsp` dances with pushes/locals. Locals live at negative offsets (`-8(%rbp)`), args that spilled at positive ones. `leave` (= `mov %rbp,%rsp; pop %rbp`) tears down, `ret` pops home. Stacks grow *down*: deeper calls = smaller `%rsp`. See [stack](../../glossary/terms.md#stack), [register](../../glossary/terms.md#register).

## Simulate It (host C — watch depth move rsp, no ASM yet)

Full program: `code/depth.c`. Samples `%rsp` at three nesting levels via the ASM helper.

```c
#include <stdio.h>

unsigned long get_rsp(void);

static void level2(void) {
    char pad[64];
    (void)pad;
    printf("level2 rsp=0x%lx\n", get_rsp());
}

static void level1(void) {
    char pad[64];
    (void)pad;
    printf("level1 rsp=0x%lx\n", get_rsp());
    level2();
}

int main(void) {
    printf("main   rsp=0x%lx\n", get_rsp());
    level1();
    return 0;
}
```

What this does: prints the live stack pointer at increasing call depth, proving frames nest downward with ~equal strides.

| Lines | Code | Why it exists |
|---|---|---|
| 3 | `get_rsp()` decl | same 3-line helper shape as 02/03 (`movq %rsp,%rax`); reads whose-stack live |
| 5–9 | `level2` + `pad[64]` | 64-byte local forces a real frame; `(void)pad` silences unused warning without optimizing the frame away |
| 11–16 | `level1` calls `level2` | nesting = two frames alive at once; print order shows entry sequence |
| 19–22 | `main` prints then dives | baseline sample; compare the three hex values (each deeper call is lower) |

Change X → Y: change both `pad[64]` to `pad[256]`. Verify: `make run` gaps widen by ~192 each (proves locals *are* the frame size — `%rsp` moves by what you declare).

## Build It (AT&T frame, assembled + traced — Linux/WSL/Docker for execution)

Standard prologue/epilogue, System V (same ABI note as P-02/01: assembles anywhere, runs on Linux):

```asm
.text
.globl frame_add
frame_add:
    pushq %rbp
    movq %rsp, %rbp
    movl %edi, -4(%rbp)
    movl %esi, -8(%rbp)
    movl -4(%rbp), %eax
    addl -8(%rbp), %eax
    popq %rbp
    ret
```

What this does: spills both int args into its own frame, reloads, adds, tears down — the canonical shape `gcc -O0` emits, handwritten so every line is yours.

| Lines | Code | Why it exists |
|---|---|---|
| 4 | `pushq %rbp` | save caller's anchor (callee-saved); stack now holds old `%rbp` on top |
| 5 | `movq %rsp, %rbp` | anchor *this* frame; locals addressed stably off `%rbp` even as `%rsp` moves |
| 6–7 | `movl ... -4(%rbp)` | spill regs to frame slots; `-4`/`-8` = below anchor (stack grows down) |
| 8–9 | reload + `addl` | compute from frame (slow, legible — `-O2` would skip the spill; `-O0` thinking) |
| 10–11 | `popq %rbp; ret` | restore caller anchor, pop home address into `%rip` |

Change X → Y: replace `popq %rbp` with `movq %rbp, %rsp; popq %rbp` (explicit `leave`). Verify: identical output (proves `leave` is exactly those two moves — use whichever reads clearer).

| AT&T here | Intel elsewhere | Note |
|---|---|---|
| `movq %rsp, %rbp` | `mov rbp, rsp` | `q` = 64-bit anchors |
| `-4(%rbp)` | `[rbp-4]` | parens = dereference + offset |
| `pushq %rbp` | `push rbp` | grows down by 8, stores old anchor |

Caller + GDB backtrace:

```c
#include <stdio.h>
int frame_add(int a, int b);

int main(void) {
    int r = frame_add(30, 12);
    printf("frame_add(30,12)=%d\n", r);
    return r != 42;
}
```

What this does: calls the framed function like any C helper; exit 0 iff the frame round-tripped args correctly.

```bash
make run
gdb -batch -ex 'break frame_add' -ex run -ex bt -ex 'x/4xg $rsp' -ex continue ./build/frames
```

What this does: stops at prologue, prints the call chain (`bt` = main → frame_add), dumps 4 stack words, finishes.

| Lines | Code | Why it exists |
|---|---|---|
| `bt` | backtrace | each line = one live frame; top return address = where `ret` goes |
| `x/4xg $rsp` | examine stack | `4` words, `x` hex, `g` giant(8-byte): expect saved `%rbp` + return address on top |

Change X → Y: add `-ex 'info registers rbp rsp'` before continue. Verify: `rbp > rsp` by a small frame size (anchor above live top — the diagram, numeric).

## Use It (Linux)

Frames explain real crash output:

```bash
./build/frames; echo "exit=$?"
gcc -g -O0 -Wall -o /tmp/frames-g main.c frame.s && gdb -batch -ex run -ex bt /tmp/frames-g 2>&1 | tail -5
```

What this does: runs cleanly, then shows what a *debug* build's backtrace looks like — the shape every segfault report wears.

| Lines | Code | Why it exists |
|---|---|---|
| `exit=$?` | proof | `0` from `r != 42` being false |
| `-g -O0` | debug build | `-g` = line info for `bt`, `-O0` = keep textbook frames (production `-O2` inlines/fuses them) |

Change X → Y: rebuild with `-O2 -fomit-frame-pointer`, rerun `bt`. Verify: fewer `%rbp` anchors (optimized code uses `%rsp`-relative addressing — faster, harder to read; why OS entry paths often keep frames).

## Ship It

Artifact: `outputs/stack-card.md` — prologue/epilogue pair, `bt` + `x/` recipes, red-zone warning (x86-64's 128 bytes below `%rsp` that signal handlers must respect — named here, fenced in P-02/04). Reuse for every future crash.

## Exercises

1. Easy — add `level3`, show three descending `rsp` values + explain the stride (frame + pad + call overhead).
2. Medium — in GDB at `frame_add`, run `x/2xg $rbp` (saved rbp + return addr) and `p/x *(int*)($rbp-4)` (spilled arg). Map each to the prologue lines.
3. Hard — write `frame_sub` by editing only the `addl` line, call both, prove no cross-talk (frames isolate — the invariant threads depend on).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| stack | per-task frames; `%rsp` = live top, `%rbp` = frame anchor | ../../glossary/terms.md#stack |
| register | `%rbp/%rsp/%rip` run the dance; rest ride along | ../../glossary/terms.md#register |
| address | return addresses live on the stack (why overflow = control) | ../../glossary/terms.md#address |

## Further Reading

- MIT 6.1810 frame notes — same prologue, 64-bit full version.
- `man 1 gdb` `bt`, `x/`, `info frame` — the 3 verbs for all future crashes.
- xv6 `kernel/swtch.S` re-read — now the pushes read as "save a frame", not noise.

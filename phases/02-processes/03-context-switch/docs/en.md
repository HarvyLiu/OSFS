# Context Switch — Stack Surgery in 30 Lines

> Pausing is saving registers. Resuming is restoring them. The stack holds the rest.

**Type:** Build
**Languages:** C, ASM
**Prerequisites:** 02-fork-exec, 01-registers-mov
**College ref:** OSTEP Ch.4–6 (limited direct execution + switch), MIT 6.1810 Lec 3 (trapframe/switch), xv6 file kernel/swtch.S + kernel/proc.h
**Time:** ~90 minutes

## Learning Objectives
- Trace save-old/restore-new as register moves plus a stack-pointer swap
- Implement a round-robin task sim with explicit PCB structs on host C
- Explain callee-saved vs caller-saved and why `%rsp` defines "whose stack"
- Connect `get_rsp()` samples to the real `swtch()` you'll write bare-metal in Phase 10

## Concept in 60s

![ctx switch](../figures/ctx-switch.svg)

<!-- source: ../figures/ctx-switch.excalidraw — open in excalidraw.com to redraw -->

Each task owns a [PCB](../../glossary/terms.md#pcb) (pid, state, saved `%rsp`, trapframe) plus its own [stack](../../glossary/terms.md#stack). Switching = (1) push live [registers](../../glossary/terms.md#register) onto old stack, (2) stash old `%rsp` in old PCB, (3) load new `%rsp` from new PCB, (4) pop new registers, (5) `ret` into new code. `fork()` created the cards; the scheduler deals them. Interrupts (`cli/sti`) fence the critical middle — P-02/04 topic, named here only.

## Simulate It (host C — scheduler you can read top to bottom)

Full program: `code/ctx.c` + `code/switch.s` (`get_rsp`). No QEMU, no root.

```c
#include <stdio.h>

unsigned long get_rsp(void);

typedef struct { int pid; int state; int step; unsigned long rsp; } pcb_t;
#define RUNNABLE 0
#define RUNNING 1

static int next_task(int cur, int n) { return (cur + 1) % n; }

static void run_slice(pcb_t *p) {
    p->state = RUNNING;
    p->rsp = get_rsp();
    printf("task %d step %d rsp=0x%lx\n", p->pid, p->step, p->rsp);
    p->step++;
    p->state = RUNNABLE;
}

int main(void) {
    pcb_t tasks[2] = {{1, RUNNABLE, 0, 0}, {2, RUNNABLE, 0, 0}};
    int cur = 0;
    for (int tick = 0; tick < 6; tick++) {
        run_slice(&tasks[cur]);
        cur = next_task(cur, 2);
    }
    printf("done steps=%d,%d\n", tasks[0].step, tasks[1].step);
    return 0;
}
```

What this does: round-robins two PCBs for 6 ticks, sampling the live stack pointer each slice — a scheduler skeleton with observable stacks.

| Lines | Code | Why it exists |
|---|---|---|
| 3 | `get_rsp()` decl | ASM helper (below); reads `%rsp` so C can *see* whose stack runs |
| 5 | `pcb_t` | mini index card: identity + liveness + progress + saved stack ptr (xv6 `struct proc` preview) |
| 9 | `next_task` | `(cur+1)%n` = round-robin; the entire "scheduling policy" for this lesson (MLFQ/CFS come in Phase 03) |
| 11–17 | `run_slice` | RUNNING → sample → print → count → RUNNABLE; real kernels add register save/restore around this |
| 20–26 | tick loop | 6 slices alternate 1,2,1,2…; proves fairness mechanically, not by assertion |

Change X → Y: change `tick < 6` to `tick < 2`. Verify: `make run` prints 2 task lines + `done steps=1,1` (proves the loop, not the data, drives interleaving).

The 3-line ASM behind `get_rsp` (AT&T):

```asm
.text
.globl get_rsp
.type get_rsp, @function
get_rsp:
    movq %rsp, %rax
    ret
```

What this does: copies the stack pointer into the return register — C's `unsigned long` comes home in `%rax`.

| Lines | Code | Why it exists |
|---|---|---|
| `movq %rsp, %rax` | copy 64-bit (`q`) stack ptr to return reg | `mov` copies (source keeps value); `%rsp` = "whose stack am I on" |
| `ret` | back to C | return value already staged in `%rax` per calling convention (P-02/01) |

Change X → Y: call `get_rsp()` twice in one function with an extra `{ char pad[64]; }` block between. Verify: second sample is lower (stack grows down on x86 — you just watched growth direction).

What a *real* switch adds (illustration — assembled to `.o` for syntax proof, executed bare-metal in Phase 10):

```asm
# shape of xv6 swtch.S: save old callee-saved, swap rsp, restore new
# old_rsp_ptr in %rdi, new_rsp in %rsi  (preview, see code/switch.s)
# pushq %rbx; pushq %rbp; pushq %r12; pushq %r13; pushq %r14; pushq %r15
# movq %rsp, (%rdi)     # stash old stack pointer into old PCB
# movq %rsi, %rsp       # load new stack pointer from new PCB
# popq %r15; ...; popq %rbx
# ret                   # land in new task (return address was on its stack)
```

What this does: the 5-step dance from Concept — push, stash, load, pop, land — with callee-saved registers (the ones functions promise to preserve) as the payload.

| Lines | Code | Why it exists |
|---|---|---|
| `pushq callee-saved` | save old task's guts on *its* stack | caller-saved regs already spilled by compiler or live only in trapframe; callee-saved must survive calls |
| `movq %rsp,(%rdi)` / `movq %rsi,%rsp` | the actual switch | two moves = whole OS trick: which stack `%rsp` names *is* which task runs |
| `popq + ret` | become new task | new stack's top holds its return [address](../../glossary/terms.md#address); `ret` jumps there |

Change X → Y: mentally drop the `pushq` block. Verify by reasoning + GDB later: new task inherits old `%rbx` — silent corruption (this is why the list is *callee-saved-complete*, never partial).

| AT&T here | Intel elsewhere | Note |
|---|---|---|
| `movq %rsp, %rax` | `mov rax, rsp` | `q` = 64-bit; our pointers are 8 bytes |
| `movq %rsp, (%rdi)` | `mov [rdi], rsp` | parens = dereference in AT&T |

## Build It (assemble the shape + run the sim)

```bash
make run
cc -Wall -Werror -c switch.s -o build/switch.o && nm build/switch.o | grep -E "get_rsp|ctx_"
```

What this does: runs the scheduler sim, then proves the ASM (helper + illustrated switch comments) assembles and exports its symbols.

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | scheduler proof | expect alternating `task 1`/`task 2` lines; `rsp=` repeats at equal call depth (same frame shape every slice — determinism you can rely on; the pad exercise in Exercises 2 moves it) |
| `cc -c switch.s` | syntax proof | `-c` = assemble only; catches `%`/`$`/suffix slips before QEMU ever boots |
| `nm ... get_rsp` | symbol proof | `T get_rsp` = linkable; lowercase would mean hidden |

Change X → Y: delete `.globl get_rsp`, rebuild. Verify: `undefined reference to get_rsp` at link — proves `.globl` is the handshake between ASM and C (same failure as Tooling 02's `_start` without `.globl`).

## Use It (Linux)

Context switches are countable on real Linux:

```bash
./build/ctx | head -4
pidstat -w 1 2 2>/dev/null | head -8 || vmstat 1 2 | head -5
grep -c processor /proc/cpuinfo
```

What this does: shows your sim's interleaving, then the host's *involuntary* switch rate (real scheduler dealing real cards).

| Lines | Code | Why it exists |
|---|---|---|
| `./build/ctx` | voluntary interleaving | your loop chose every switch (cooperative); Linux usually preempts (timer-driven) |
| `pidstat -w / vmstat` | switch counters | `cswch/s` = voluntary+involuntary per second; compare idle vs `make -j` load |
| `/proc/cpuinfo` | how many stacks can truly run at once | one core = one `%rsp` live; more tasks than cores = switching is mandatory, not optional |

Change X → Y: run `vmstat 1 2` idle vs while `yes > /dev/null & vmstat 1 2; kill %1`. Verify: `cs` column jumps under load — the number your Phase-03 schedulers will try to keep *useful*, not just high.

## Ship It

Artifact: `outputs/runbook-ctx.md` — "is it the switch?" triage (wrong task resumes → rsp stash/load swapped; callee-saved garbage → missing push/pop pair; first-switch crash → new stack missing seeded return address). Reuse in Phase 10's `swtch` bring-up verbatim.

## Exercises

1. Easy — add a third task, run 6 ticks, show `steps=2,2,2` (proves `next_task` generalizes via `%n`).
2. Medium — print `get_rsp()` in `main` vs inside `run_slice` with a `char pad[128]` in one. Explain the numeric gap (frames + growth direction).
3. Hard — seed a fake stack in an array (`unsigned long fake[64]`, set top to a function address) and describe what `movq %rsi,%rsp; ret` would do with it (this *is* thread creation — implemented for real in capstone).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| PCB | card per task: pid, state, saved rsp, trapframe | ../../glossary/terms.md#pcb |
| register | what gets pushed/popped; `%rsp` picks the stack | ../../glossary/terms.md#register |
| stack | per-task call memory; switch = swap which one `%rsp` names | ../../glossary/terms.md#stack |
| syscall | the fenced gate that will trigger kernel-side switches | ../../glossary/terms.md#syscall |

## Further Reading

- OSTEP Ch.6 — limited direct execution + the switch it performs around.
- xv6 `kernel/swtch.S` (15 lines) + `kernel/proc.h` — read both now; every line is named above.
- `man 1 pidstat,vmstat` — the counters used in Use It.

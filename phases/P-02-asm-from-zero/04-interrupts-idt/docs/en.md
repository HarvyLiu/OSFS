# Interrupts, cli/sti, IDT Preview — The CPU Knocking on Your Door

> Polling asks. Interrupts tell. The IDT is the phone book; cli hangs up temporarily.

**Type:** Learn
**Languages:** C, ASM
**Prerequisites:** 03-lea-addressing
**College ref:** OSTEP Ch.6 (limited direct execution needs traps), MIT 6.1810 Lec 4 (traps/IDT), xv6 file kernel/trap.c + kernel/trampoline.S (the real path)
**Time:** ~75 minutes

## Learning Objectives
- Trace device → CPU → IDT slot → handler → `iret` using a dispatch-table model
- Implement an IDT-shaped table (array of function pointers) with mask (`cli`) semantics in C
- Explain `cli`/`sti`, `pushal`-style saving, and why handlers must be short
- Connect this preview to the scheduler (timer tick preempts) and syscalls (trap 64+ path)

## Concept in 60s

![interrupt path](../figures/interrupt-path.svg)

<!-- source: ../figures/interrupt-path.excalidraw — open in excalidraw.com to redraw -->

Hardware raises IRQ n. If interrupts are enabled (`sti` state), the CPU looks up slot n in the IDT (Interrupt Descriptor Table — 256 entries, each "run this code at this privilege"), pushes [registers](../../../../glossary/terms.md#register) + flags, jumps to the handler. Handler saves the rest, does minimal work (ack device, wake a task), restores, `iret` pops back to the interrupted code — which never knew it paused. `cli` clears the enable flag: IRQs wait (pending), they don't vanish. Non-maskable few excepted. [Syscalls](../../../../glossary/terms.md#syscall) ride the same rails (software-raised trap instead of wire-raised IRQ).

## Simulate It (host C — IDT as an array, no QEMU)

Split across `code/idt.h` + `code/idt.c` + `code/demo.c` (P-01/06 practice, on purpose).

```c
// idt.h -- the phone book shape.
#ifndef OSFS_IDT_H
#define OSFS_IDT_H

#define NIRQ 8
typedef void (*handler_t)(void);
void idt_register(int n, handler_t h);
void idt_raise(int n);       // deliver if enabled, else mark pending
void irq_enable(void);       // sti model
void irq_disable(void);      // cli model
int irq_pending(int n);

#endif
```

What this does: publishes an 8-slot table plus enable/disable/pending — the exact verbs real IDT code needs, minus privilege bytes.

| Lines | Code | Why it exists |
|---|---|---|
| 5–6 | `NIRQ` + `handler_t` | 8 slots keeps traces tiny (real: 256); function pointer = "code to jump to" |
| 7–11 | five verbs | register/deliver/enable/disable/query — handlers, `cli/sti`, and pending-bit checks each map to one |

```c
// idt.c -- delivery with masking.
#include "idt.h"

static handler_t table[NIRQ];
static int enabled = 1;
static int pending[NIRQ];

void idt_register(int n, handler_t h) { if (n >= 0 && n < NIRQ) table[n] = h; }
void irq_enable(void) { enabled = 1; }
void irq_disable(void) { enabled = 0; }
int irq_pending(int n) { return (n >= 0 && n < NIRQ) ? pending[n] : 0; }

void idt_raise(int n) {
    if (n < 0 || n >= NIRQ || !table[n]) return;
    if (!enabled) { pending[n] = 1; return; }
    pending[n] = 0;
    table[n]();
}
```

What this does: delivers immediately when enabled, parks as pending when masked — `cli`'s whole contract in 6 lines.

| Lines | Code | Why it exists |
|---|---|---|
| 3–5 | statics | table + flag + pending bits: zero-init = no handlers, enabled, nothing pending (sane reset state) |
| 7 | bounds-checked register | IRQ numbers are hardware-fixed; garbage `n` must not index wild (same guard real `idtinit` needs) |
| 13–17 | raise | no-handler = ignore (spurious IRQ reality); masked = remember, don't run; enabled = clear-then-call (clear *first* so re-entrant raises re-pend instead of recursing) |

```c
// demo.c -- timer tick wakes a counter.
#include <stdio.h>
#include "idt.h"

static int ticks = 0;
static void on_tick(void) { ticks++; }

int main(void) {
    idt_register(0, on_tick);
    irq_enable();
    idt_raise(0);
    idt_raise(0);
    printf("ticks=%d (enabled)\n", ticks);
    irq_disable();
    idt_raise(0);
    printf("ticks=%d pending=%d (masked)\n", ticks, irq_pending(0));
    irq_enable();
    if (irq_pending(0)) { idt_raise(0); }
    printf("ticks=%d after unmask\n", ticks);
    return ticks != 3;
}
```

What this does: fires twice live, once masked (parks), then delivers the parked one after `sti` — the pending lifecycle, observable in 3 prints.

| Lines | Code | Why it exists |
|---|---|---|
| 8–11 | two live raises | `ticks` 0→2: immediate delivery path (the common case) |
| 12–14 | masked raise | `ticks` stays 2, `pending` reads 1: `cli` defers, hardware holds the line |
| 15–17 | unmask + drain | real kernels check-and-deliver on `sti`; the parked tick lands → 3 (nothing lost, just late) |

Change X → Y: register nothing for IRQ 3, raise it enabled. Verify: no crash, no count change (proves the no-handler guard — spurious IRQs are real and must be boring).

## Build It (AT&T shape — assemble-only; `cli` faults in userspace!)

Never execute `cli` on host Linux (privileged — your process dies). Assemble the shape, run the C:

```asm
# irq.s -- real handler skeleton (reference; assembled, not called on host).
# On bare metal: vector stub -> save regs -> call C handler -> restore -> iret
.text
.globl irq_shape_note
irq_shape_note:
    ret
# cli                   # clear interrupt flag: IRQs pend from here
# pushq %rax; pushq %rcx; ...   # save caller-scratch (callee-saved ride in C call)
# call irq_dispatch     # C: table[vector]()
# popq ...              # restore in reverse
# sti                   # re-enable (or iret restores flags incl. IF)
# iretq                 # pop rip/cs/rflags (+rsp/ss if ring change) -> resume
```

What this does: documents the 6-step bare-metal dance next to a symbol that proves the file assembles — read it as the Phase-10 checklist.

| Lines | Code | Why it exists |
|---|---|---|
| `cli` position | first, before save | a second IRQ mid-save would corrupt the frame being built (fence first, work second) |
| push/call/pop | [registers](../../../../glossary/terms.md#register) round-trip | interrupted code resumes bit-identical (the context-switch promise, per-interrupt) |
| `iretq` (not `ret`) | pops flags + far return | restores interrupt-flag state atomically — `sti`+`ret` separately would race a window |

Change X → Y: mentally move `cli` after the pushes. Verify by reasoning: nested IRQ lands mid-frame → double-push chaos (this ordering bug bricked real bring-ups; fence-first is law).

```bash
make run
cc -Wall -c irq.s -o build/irq.o && nm build/irq.o | grep irq_shape_note
```

What this does: runs the C lifecycle, then proves the ASM shape assembles and exports its marker.

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | lifecycle proof | expect `ticks=2`, `pending=1`, `ticks=3 after unmask` |
| `-c irq.s` + `nm` | syntax proof | privileged lines are comments (safe); `T irq_shape_note` = file linked into the lesson honestly |

Change X → Y: uncomment `cli` and try running it (don't — read first): userspace `cli` = `#GP` fault, `SIGSEGV`, core dump (the experiment that teaches rings faster than any paragraph; observe via `dmesg` curiosity only).

## Use It (Linux)

Real interrupt lines are listed live:

```bash
cat /proc/interrupts | head -12
grep -E "timer|keyboard|virtio" /proc/interrupts | head -5
cat /proc/irq/default_smp_affinity 2>/dev/null
```

What this does: shows IRQ numbers, per-CPU counts, device names — your 8-slot model wearing production clothes (256 lines, same idea).

| Lines | Code | Why it exists |
|---|---|---|
| `/proc/interrupts` head | the table | `CPU0 CPU1 ...` columns = per-core delivery counts ticking up (watch `timer` race) |
| `grep timer` | the tick | `LOC`/`timer` rows firing thousands/sec = the preemption source your schedulers assume |
| `default_smp_affinity` | routing | which CPUs may take each IRQ (SMP preview — one line, whole Phase 10 topic) |

Change X → Y: `watch -n1 'cat /proc/interrupts | head -8'` for 5 seconds idle vs while `yes > /dev/null`. Verify: counts climb faster under load (interrupts follow work — the feedback loop power management rides).

## Ship It

Artifact: `outputs/irq-card.md` — fence-first order, save/call/restore/`iret` checklist, pending-drain rule, userspace-never-`cli` warning. Bring it to Phase 10's `trap.c` day one.

## Exercises

1. Easy — add IRQ 1 keyboard counter; raise mixed sequence `0,1,0,1`; show both counts (table holds *vectors*, not one bell).
2. Medium — implement pending-drain inside `irq_enable` (loop pending→deliver). Re-run demo minus manual drain; prove same `ticks=3` (real `sti` paths do this work).
3. Hard — add priority: IRQ 0 preempts IRQ 1's handler (nested `idt_raise` inside a handler with enabled flag set). Show nesting depth 2 then argue why real kernels defer most work to bottom halves instead (stack depth + latency — preview of softirq/tasklet).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| register | what stubs save; what `iret` restores alongside flags | [register](../../../../glossary/terms.md#register) |
| stack | handler frames nest on the interrupted stack (or IST in 64-bit) | [stack](../../../../glossary/terms.md#stack) |
| syscall | software trap through the same descriptor table (vector ~0x80/64+) | [syscall](../../../../glossary/terms.md#syscall) |

## Further Reading

- MIT 6.1810 traps lecture + `kernel/trap.c` — map each function to a demo verb above.
- OSDev IDT + Interrupts — descriptor bytes + PIC/APIC init (bring-up companion).
- `man 5 proc` (`/proc/interrupts` section) — every column defined.

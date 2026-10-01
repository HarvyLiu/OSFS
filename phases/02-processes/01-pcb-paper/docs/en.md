# PCB — The Process on Paper

> Pause a process and you must remember everything: registers, stack, state, files, space. That memory is the PCB.

**Type:** Learn
**Languages:** C
**Prerequisites:** 05-structs-pcb
**College ref:** OSTEP Ch.4 (process anatomy), MIT 6.1810 `struct proc` (fields tour), xv6 file kernel/proc.h (the real card, now legible)
**Time:** ~60 minutes

## Learning Objectives
- List every field a pause must preserve (trapframe regs + pc/sp/flags, state, pid/parent, files, root)
- Implement save/clobber/restore round-trip over a trapframe struct in C
- Explain trapframe (user regs at trap) vs kernel stack vs PCB (the three per-process memories)
- Connect the card to fork (copies it), switch (swaps it), wait (reaps it)

## Concept in 60s

![trapframe](../figures/trapframe.svg)

<!-- source: ../figures/trapframe.excalidraw — open in excalidraw.com to redraw -->

Pausing = saving: 16 general [registers](../../../../glossary/terms.md#register) + program counter + [stack](../../../../glossary/terms.md#stack) pointer + flags (the trapframe — pushed by stub + handler on entry). Plus scheduler state (RUNNABLE/RUNNING/ZOMBIE), identity (pid/parent/name), resources (open files, address-space root, cwd), accounting (ticks, exit code). Resume = restoring in reverse. The PCB *is* this card; the table of cards *is* the process list; `fork` photocopies a row (new pid, shared-then-COW space); `wait` deletes one (after collecting the exit code). P-01/05 built the shape; this lesson fills every drawer.

## Simulate It (host C — save/clobber/restore, no QEMU)

Full program: `code/pcb.c`. Eight "registers", trap entry, chaos, resume.

```c
#include <stdio.h>
#include <string.h>

#define NREG 8

typedef struct {
    long regs[NREG];   // r0..r7 stand-ins (real: rax..r15)
    long pc;           // where to resume
    long sp;           // whose stack
    long flags;        // interrupt + condition state
} trapframe_t;

typedef enum { UNUSED = 0, RUNNABLE, RUNNING, ZOMBIE } state_t;

typedef struct {
    int pid;
    int ppid;
    state_t state;
    trapframe_t tf;
    unsigned long pageroot; // address-space root (05/03's root!)
    char name[16];
} pcb_t;

static void trap_save(pcb_t *p, long pc, long sp) {
    for (int i = 0; i < NREG; i++) p->tf.regs[i] = 100 + i; // stand-in "live" values
    p->tf.pc = pc;
    p->tf.sp = sp;
    p->tf.flags = 0x202; // IF set: interrupts were enabled
}

static void clobber(pcb_t *p) { // the handler/scheduler runs: regs trashed
    for (int i = 0; i < NREG; i++) p->tf.regs[i] = -1;
    p->tf.pc = 0;
}

static void trap_restore(pcb_t *p, const trapframe_t *saved) {
    p->tf = *saved; // struct copy = resume (real: pop in reverse)
}

int main(void) {
    pcb_t p;
    memset(&p, 0, sizeof p);
    p.pid = 7;
    p.ppid = 1;
    p.state = RUNNING;
    p.pageroot = 0x100000;
    snprintf(p.name, 16, "%s", "demo");
    trap_save(&p, 0x400100, 0x7fff00);
    trapframe_t snap = p.tf;        // snapshot = what the switch stores aside
    clobber(&p);                    // scheduler/handler trashes everything
    trap_restore(&p, &snap);        // resume path
    printf("pid=%d ppid=%d state=%d pc=0x%lx sp=0x%lx r0=%ld r7=%ld root=0x%lx name=%s\n",
           p.pid, p.ppid, p.state, p.tf.pc, p.tf.sp,
           p.tf.regs[0], p.tf.regs[7], p.pageroot, p.name);
    return p.tf.pc != 0x400100 || p.tf.regs[7] != 107;
}
```

What this does: saves a fake trap state, snapshots it, trashes the live copy (what kernel work looks like to user regs), restores bit-exact — pause/resume without hardware.

| Lines | Code | Why it exists |
|---|---|---|
| 5–10 | `trapframe_t` | regs+pc+sp+flags: the *resume set* (anything missing here is forgotten on every switch — audit this struct first in any port) |
| 12–21 | `pcb_t` | identity + state + trapframe + root + name (P-01/05's card grown its two OS sections: CPU state and space root) |
| 23–29 | `trap_save` | stub+handler model: capture pc/sp/flags + live regs (`0x202` = IF: resuming re-enables interrupts — flags ride along, not apart) |
| 31–35 | `clobber` | stands in for handler/scheduler execution (proves the snapshot, not luck, carries the resume — delete `snap` and watch it fail) |
| 37–39 | struct-copy restore | real hardware pops in reverse order; C copies whole (same bytes, less ceremony — the *semantics* under test, not the pops) |
| 42–53 | lifecycle | zero-init → identity → save → snap → trash → restore → verify-every-field (exit code gates on pc AND a register — partial restores fail) |

Change X → Y: change `snap` to restore from a *second* PCB's frame (cross-resume). Verify: pc/regs print the *other* task's values (that's literally a context switch — same copy, different source; 02/03's mechanism in C clothes).

## Build It

```bash
make run
make test
```

What this does: runs the round-trip, then asserts save fidelity, clobber-then-restore, state/ppid/root/name carriage, and table find/reap (P-01/05 verbs, grown card).

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | expect `pc=0x400100 ... r0=100 r7=107 root=0x100000 name=demo` (every drawer opened, contents correct) |
| `make test` | machine proof | round-trip + isolation (two PCBs don't cross-talk — table rows are independent by construction) |

Change X → Y: skip `memset` (uninitialized PCB). Verify: name/pageroot print garbage (the zero-init habit from P-01/05, re-earned — kernels zero pages before mapping for exactly this + security).

## Use It (Linux)

Your kernel keeps these cards enumerable:

```bash
ps -o pid,ppid,stat,wchan:20,comm -p 1
cat /proc/1/status | grep -E "State|PPid|VmRSS|Threads"
```

What this does: shows init's row (state, wait-channel, command = your `name[16]`), its status fields (ppid, memory, thread count = extra rows), proving cards exist per process+thread.

| Lines | Code | Why it exists |
|---|---|---|
| `wchan` | wait channel | *where* a sleeping process parks (the scheduler's own words for "which queue I'm on" — state + location, together) |
| `status` rows | card dump | State/PPid/VmRSS/Threads = your struct's fields wearing procfs clothes (map each to a drawer) |

Change X → Y: `cat /proc/$$/status | grep -E "State|PPid"` (your shell). Verify: `PPid` chains upward (cards link into the tree `fork` built — traverse to 1 by hand, it's short).

## Ship It

Artifact: `outputs/trapframe.h` — `trapframe_t` + `pcb_t` + state enum as Phase 10's drop-in (grow regs to 16+, add files/cwd then). Plus save/restore checklist (order: save-all → work → restore-reverse — the stub law from P-02/04, cited).

## Exercises

1. Easy — add `long ticks` + `exitcode`, run 3 fake slices incrementing ticks, exit with code, reap and print it (accounting drawer, wired end to end).
2. Medium — two PCBs ping-pong restores (A snap → B snap → restore A → verify A's regs, not B's — cross-talk hunt; passes iff snapshots isolate).
3. Hard — `fork`-copy: duplicate a PCB with new pid, shared pageroot, COW-marked flag field (add `cow:1`); describe what `exec` must then clear (space + regs reset, pid kept — the trio's field-level story, told per drawer).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| PCB | the card: identity + state + trapframe + root + files | [pcb](../../../../glossary/terms.md#pcb) |
| register | trapframe contents (pause = save these; resume = restore) | [register](../../../../glossary/terms.md#register) |
| stack | `sp` field + kernel stack per task (two stacks per process in kernels!) | [stack](../../../../glossary/terms.md#stack) |
| syscall | trap path fills trapframes (user regs arrive via this gate) | [syscall](../../../../glossary/terms.md#syscall) |

## Further Reading

- xv6 `kernel/proc.h` — the full card (context + trapframe + fields mapped 1:1 to above).
- OSTEP Ch.4 — process anatomy (the chapter this lesson itemizes).
- `man 1 ps` (`stat`, `wchan`) + `man 5 proc` (status rows — card readers).

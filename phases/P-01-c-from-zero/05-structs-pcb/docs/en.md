# structs, enums, typedef — The PCB on Paper

> A struct groups bytes. An enum names states. Together they are every kernel table.

**Type:** Learn
**Languages:** C
**Prerequisites:** 04-malloc-heap
**College ref:** OSTEP Ch.4 (the PCB), MIT 6.1810 `struct proc` (fields preview), xv6 file kernel/proc.h (the real card)
**Time:** ~75 minutes

## Learning Objectives
- Trace struct layout (offsets, `sizeof`, padding) using a field map
- Implement a PCB table: init, find-by-pid, state transitions with an enum
- Explain `->` vs `.`, `typedef`, and why enums beat raw ints for states
- Connect this table to fork (creates a row), scheduler (picks a row), wait (reaps a row)

## Concept in 60s

![struct layout](../figures/struct-layout.svg)

<!-- source: ../figures/struct-layout.excalidraw — open in excalidraw.com to redraw -->

You need a roster the OS can scan, so you build it from grouped bytes. A `struct` lays fields side by side: `pid` at offset 0, `state` next, then `rsp`, then a name buffer. `sizeof` includes *padding* — alignment gaps the compiler inserts, which `pahole` and offset prints reveal. `p->pid` means "the pid field of the struct at [address](../../../../glossary/terms.md#address) p", sugar for `(*p).pid`. An `enum` names the states (`UNUSED=0, RUNNABLE, RUNNING, ZOMBIE`) so `p->state == ZOMBIE` reads like English and `switch` warns on missed cases. A `typedef` shortens the name to `pcb_t`. The OS keeps *arrays* of these rows: the process table. See [PCB](../../../../glossary/terms.md#pcb), [pointer](../../../../glossary/terms.md#pointer), [heap](../../../../glossary/terms.md#heap).

## Simulate It (host, no QEMU)

You run a process table in miniature. `code/pcb.c` holds 4 slots, spawns 2, runs one, zombies one, then reaps it.

```c
#include <stdio.h>
#include <string.h>

typedef enum { UNUSED = 0, RUNNABLE, RUNNING, ZOMBIE } state_t;

typedef struct {
    int pid;
    state_t state;
    unsigned long rsp;
    char name[16];
} pcb_t;

#define NPROC 4
static pcb_t table[NPROC];

static pcb_t *alloc_pid(int pid, const char *name) {
    for (int i = 0; i < NPROC; i++)
        if (table[i].state == UNUSED) {
            table[i].pid = pid;
            table[i].state = RUNNABLE;
            table[i].rsp = 0;
            snprintf(table[i].name, 16, "%s", name);
            return &table[i];
        }
    return 0;
}

static pcb_t *find_pid(int pid) {
    for (int i = 0; i < NPROC; i++)
        if (table[i].state != UNUSED && table[i].pid == pid)
            return &table[i];
    return 0;
}

int main(void) {
    pcb_t *a = alloc_pid(1, "init");
    pcb_t *b = alloc_pid(2, "shell");
    printf("sizeof(pcb_t)=%zu pid_off=%zu state_off=%zu\n",
           sizeof(pcb_t), __builtin_offsetof(pcb_t, pid),
           __builtin_offsetof(pcb_t, state));
    a->state = RUNNING;
    b->state = ZOMBIE;
    printf("run %d(%s) zombie %d(%s)\n", a->pid, a->name, b->pid, b->name);
    pcb_t *f = find_pid(2);
    if (f) { f->state = UNUSED; printf("reaped pid 2\n"); }
    printf("slots used=%d\n",
           (table[0].state != UNUSED) + (table[1].state != UNUSED) +
           (table[2].state != UNUSED) + (table[3].state != UNUSED));
    return 0;
}
```

What this does: owns a tiny process table end-to-end — layout printed, two rows born, one runs, one dies, one reaped — every later phase's lifecycle in miniature.

| Lines | Code | Why it exists |
|---|---|---|
| 3 | `enum state_t` | names 0–3; `UNUSED` must be 0 so static zero-init = free slot (C guarantees `static` starts zeroed) |
| 5–10 | `struct` + `typedef` | `pid/state/rsp/name` = identity/liveness/[stack](../../../../glossary/terms.md#stack)/label; `typedef` lets later code say `pcb_t` not `struct pcb` |
| 13 | `static table[NPROC]` | the process table itself; `static` = zeroed + file-private (no other file can corrupt it) |
| 15–26 | `alloc_pid` | first-fit free slot; `snprintf` (not `strcpy`) caps the name at 15+zero (P-01/02 zero rule) |
| 28–34 | `find_pid` | linear scan skipping `UNUSED`; returns [pointer](../../../../glossary/terms.md#pointer) or `0` (caller must check — like `malloc`) |
| 37–39 | `offsetof` print | byte offsets of fields: proves layout is numbers, reveals padding before it surprises you |
| 40–44 | transitions | `->` writes through the pointer into the *shared* table (no copies — scheduler and fork see the same row) |
| 45–49 | reap + count | `ZOMBIE→UNUSED` = what `wait()` does; count proves the table, not hope, tracks liveness |

Change X → Y: change `char name[16]` to `char name[8]` and spawn `"a-very-long-process-name"`. Verify: `make run` prints truncated `a-very-l` (proves `snprintf` capped it — `strcpy` would have smashed the next field).

## Build It (offsets don't lie — measure, don't guess)

```bash
make run
gcc -Wall -Werror -std=c11 -o /tmp/offsets code/offsets.c && /tmp/offsets
```

What this does: runs the table sim, then runs a second probe printing every field offset + total size so padding is visible, not rumored.

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | table proof | expect `sizeof(pcb_t)=40` (typical: 4+4+8+16+pad) — your machine's number is the spec, not mine |
| `offsets.c` | layout probe | one `offsetof` line per field; gaps between end-of-field and next-offset = padding bytes |

Change X → Y: reorder struct to `char name[16]; int pid; state_t state; unsigned long rsp;` in a scratch copy. Verify: size changes or stays by platform rules (proves order affects padding — kernels lay hot fields first deliberately).

`code/offsets.c` (the probe):

```c
#include <stdio.h>
#include <stddef.h>
#include <string.h>

typedef enum { UNUSED = 0, RUNNABLE, RUNNING, ZOMBIE } state_t;
typedef struct { int pid; state_t state; unsigned long rsp; char name[16]; } pcb_t;

int main(void) {
    printf("pid=%zu state=%zu rsp=%zu name=%zu size=%zu\n",
           offsetof(pcb_t, pid), offsetof(pcb_t, state),
           offsetof(pcb_t, rsp), offsetof(pcb_t, name), sizeof(pcb_t));
    return 0;
}
```

What this does: prints the four offsets + size in one line you can diff across compilers/platforms.

## Use It (Linux)

You cannot touch the kernel's table, but you can read its shadows:

```bash
ps -o pid,stat,comm -p 1
cat /proc/1/status | head -8
ls /proc/1/task/ | head -5
```

What this does: shows PID 1's state + name (your `name[16]` at work in the wild), its status file (state, memory, masks), and its thread list (each row = one schedulable).

| Lines | Code | Why it exists |
|---|---|---|
| `ps ... -p 1` | init's card | `STAT` letter (`Ss`) = your enum in kernel clothing; `COMMAND` = your `name` field |
| `/proc/1/status` | field dump | `State:`, `Pid:`, `PPid:`, `VmRSS:` — the grown-up `pcb_t` with memory + parentage |
| `/proc/1/task/` | thread rows | one directory per thread sharing the address space (Concurrency phase preview) |

Change X → Y: replace `1` with `$$` (your shell). Verify: `PPid` points at its parent, `State: S` (sleeping) — cards link into a tree (fork/exec lesson's `ppid`, now with fields).

## Ship It

Artifact: `outputs/pcb-template.h` — drop-in `pcb_t` + state enum + `alloc/find` signatures for Phase 10's kernel (swap `snprintf` for serial-safe copy then). Reuse: include it, implement the two functions against *your* table size.

## Exercises

1. Easy — add `int ppid` field, set shell's ppid to init's pid, print both. Show `find_pid(ppid)` resolves.
2. Medium — add `ZOMBIE` timeout: count ticks since zombified, reap only after 2 (init adopts orphans — preview of reaping policy).
3. Hard — replace the array with a malloc'd table of `N` (argv-sized), handle OOM, free at exit with ASan clean (P-01/04 ownership meets PCB).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| PCB | one struct row per process; table = the OS's roster | [pcb](../../../../glossary/terms.md#pcb) |
| address | `&table[i]` locates row `i`; `->` reaches its fields | [address](../../../../glossary/terms.md#address) |
| heap | where a grown-up table lives when N isn't known at compile time | [heap](../../../../glossary/terms.md#heap) |

## Further Reading

- xv6 `kernel/proc.h` — the real 20-field card; every field now parses.
- OSTEP Ch.4 — process API atop these exact transitions.
- `man 1 ps` `STAT` codes — the kernel's public enum for your private one.

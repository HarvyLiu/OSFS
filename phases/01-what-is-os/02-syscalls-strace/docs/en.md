# Syscalls, strace Your First Trap — Crossing Into the Kernel

> printf whispers to the library. write() knocks on the kernel. strace eavesdrops on every knock.

**Type:** Build
**Languages:** C
**Prerequisites:** 01-abstractions
**College ref:** OSTEP Ch.6 (system calls + traps), MIT 6.1810 Lec 1 (ecall path), xv6 file kernel/syscall.c (the dispatch table)
**Time:** ~75 minutes

## Learning Objectives
- Trace `write()` from C call → trap → kernel handler → return using `strace` output
- Implement fd-level I/O (`open`/`write`/`read`/`close`) plus `getpid()` without stdio buffering
- Explain return conventions (fd/error, bytes-or--1) and `errno` as the error channel
- Connect traps to interrupts (same rails, software-raised) and to `man 2` fluency

## Concept in 60s

![syscall trap](../figures/syscall-trap.svg)

<!-- source: ../figures/syscall-trap.excalidraw — open in excalidraw.com to redraw -->

Userspace can't touch hardware (rings forbid it — P-02/04's `cli` fault was the demo). So it *asks*: load a number (`__NR_write` = 1 on x86-64 Linux) + args into registers, execute `syscall` (the trap instruction): CPU switches to ring 0, indexes the dispatch table (`syscall.c`'s array of function pointers — an IDT for software), runs the handler with *your* args, returns with result in `%rax` (or `-errno`). Library calls (`printf`) are userspace formatting + this trap. `strace` prints each crossing: name(args) = result. See [syscall](../../glossary/terms.md#syscall).

## Simulate It (host C — fd-level I/O, portable, no strace needed)

Full program: `code/pid.c`. No `printf` for the payload (one is kept for the report line only).

```c
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int main(void) {
    long pid = (long)getpid();
    const char *path = "build/trap-demo.txt";
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open"); return 1; }
    const char *msg = "knock knock (via write)\n";
    size_t left = strlen(msg);
    while (left > 0) {
        ssize_t n = write(fd, msg + (strlen(msg) - left), left);
        if (n < 0) { perror("write"); return 1; }
        left -= (size_t)n;
    }
    close(fd);

    char buf[64];
    int rfd = open(path, O_RDONLY);
    if (rfd < 0) { perror("reopen"); return 1; }
    ssize_t nr = read(rfd, buf, sizeof(buf) - 1);
    if (nr < 0) { perror("read"); return 1; }
    close(rfd);
    buf[nr] = 0;
    printf("pid=%ld back=[%s]", pid, buf);
    return strcmp(buf, msg) != 0;
}
```

What this does: knocks three times (`open`/`write`/`read`) with raw fds, proving each trap's contract — fds, short writes, manual termination.

| Lines | Code | Why it exists |
|---|---|---|
| 7 | `getpid()` | cheapest trap: no args, returns pid (identity papers, needed by every later lesson's logs) |
| 9 | `open(...O_CREAT\|O_TRUNC, 0644)` | flags compose (`\|`): write-only + create + raze; `0644` mode (owner rw, rest r — permissions start here, Protection phase deepens) |
| 10 | `fd < 0` | errors return -1 with reason in `errno` (`perror` prints it — the error *channel* is global, the signal is -1) |
| 12–17 | write loop | `write` may write *fewer* bytes (pipes/sockets/disks do!): loop until `left==0` (first robust-IO habit — stdio hid this from you) |
| 21–25 | read + terminate | `read` returns count (not zero-terminated!): `buf[nr]=0` makes it a C string *after* (forget this → garbage prints, classic) |
| 27 | `strcmp != 0` | fidelity gate (same as 01-abstractions — traps must round-trip exactly) |

Change X → Y: replace the write loop with a single `write(fd, msg, left)` (no loop). Verify: still passes on regular files (they rarely short-write) — then read `man 2 write` RETURN VALUE to learn *when* it lies (pipes under signal — the case your loop already survives).

## Build It

```bash
make run
cat build/trap-demo.txt
```

What this does: runs the three knocks, then shows the artifact (the trap's durable effect — bytes that outlive the process).

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | expect `pid=<n> back=[knock knock (via write)]` + exit 0 |
| `cat` | durability proof | process exited, bytes remain (files outlive procs — the persistence promise FS phases keep) |

Change X → Y: `chmod 000 build/trap-demo.txt`, rerun. Verify: `open: Permission denied` (mode bits enforced *at the trap* — protection preview in one error).

## Use It (Linux-only — strace lives here)

Read your own knocks back:

```bash
strace -f -e trace=%process,%file,write,read,openat,close ./build/trapdemo 2>&1 | head -20
strace -c -e trace=write ./build/trapdemo 2>&1 | tail -5
man 2 write | sed -n '/RETURN VALUE/,/ERRORS/p' | head -12
```

What this does: traces the exact traps (names+args+results), counts them, then opens the contract at RETURN VALUE (the paragraph that governs Exercise 1's sabotage).

| Lines | Code | Why it exists |
|---|---|---|
| `-e trace=...` | filter | `%process`=fork/exec/exit family, `%file`=open family: without filters, loader/mmap noise drowns the 6 lines you care about |
| `strace -c` | summary table | `% time/calls/errors` per trap (performance intuition starts here: traps cost ~100ns–µs, batch accordingly) |
| `man 2` excerpt | contract | short-write + EINTR rules in the horse's mouth (read RETURN VALUE before every trap you newly use — habit for life) |

Change X → Y: drop `-e` entirely, count lines (`| wc -l`). Verify: 50+ lines (loader, locale, vectors — your 6 knocks hide among startup's; filters are microscopes, learn them).

## Ship It

Artifact: `outputs/strace-card.md` — `-f/-e/-c/-p/-o` verbs, `%process/%file/%network` sets, `man 2` reading order (NAME → RETURN VALUE → ERRORS), EINTR/short-write rules. The debugging pocket reference for the entire course.

## Exercises

1. Easy — print `getpid()` before/after `fork()` (after 02-fork-exec: parent/child pids differ — identity is per-process, proven in 3 lines).
2. Medium — copy a 1 MiB file with 1-byte `read`/`write` vs 64 KiB chunks, `strace -c` both (trap counts differ ~1M×; wall time differs ~100× — batching quantified, the lesson syscalls *cost*).
3. Hard — implement `full_write(fd, buf, n)` handling short writes *and* EINTR (`errno==EINTR` → retry), prove with a signal-interrupted pipe (this function ships in every serious codebase — now it's yours).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| syscall | ring-0 request via trap instruction; `strace` shows each | ../../glossary/terms.md#syscall |
| address | trap args pass pointers (kernel copies in/out — protection's first job) | ../../glossary/terms.md#address |
| heap | trap buffers often heap (`malloc`'d I/O arenas in servers) | ../../glossary/terms.md#heap |
| PCB | kernel side of `getpid` (your card, read back to you) | ../../glossary/terms.md#pcb |

## Further Reading

- `man 2 syscalls` (the list) + `man 2 write,open,read` (the three contracts used).
- OSTEP Ch.6 — limited direct execution: traps as the control-transfer arsenal.
- xv6 `kernel/syscall.c` — the dispatch array (your `strace` names index into exactly this shape).

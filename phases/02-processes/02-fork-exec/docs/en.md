# fork/exec/wait — One Process Becomes Two, Then Someone Else

> fork() clones, exec() replaces, wait() collects. That trio is process birth.

**Type:** Build
**Languages:** C
**Prerequisites:** 03-c-pointers
**College ref:** OSTEP Ch.4–5 Processes + API, MIT 6.1810 Lec 2–3 fork/exec, xv6 file kernel/proc.c
**Time:** ~75 minutes

## Learning Objectives
- Trace parent vs child return paths after `fork()` using a tree diagram
- Implement fork/exec/wait in hosted C and read exit status
- Explain copy-on-write intuition and why exec replaces the address space
- Connect PCB fields to what the kernel must save per process

## Concept in 60s

![fork tree](../figures/fork-tree.svg)

<!-- source: ../figures/fork-tree.excalidraw — open in excalidraw.com to redraw -->

A [PCB](../../../../glossary/terms.md#pcb) is the kernel's index card per process (pid, state, registers, page-table pointer, open files). `fork()` photocopies the card + memory (lazily, copy-on-write): parent gets child's pid, child gets 0 — same code, two futures. `exec()` throws away the memory and loads a new program into the same card. `wait()` lets the parent collect the child's exit code so no zombie lingers. No pointers here yet — just "who am I after the call".

## Simulate It (host Linux, no QEMU)

Full program: `code/main.c`. Requires Linux/WSL/Docker (uses `fork`). Build with `make run`.

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }
    if (pid == 0) {
        execlp("echo", "echo", "hello-from-child", (char *)NULL);
        perror("exec");
        _exit(127);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    printf("child %d exited=%d\n", pid, WEXITSTATUS(status));
    return 0;
}
```

What this does: clones the process, replaces the child with `echo`, then has the parent wait and print how the child exited.

| Lines | Code | Why it exists |
|---|---|---|
| 1–4 | includes | `unistd.h` = `fork`/`exec`, `sys/wait.h` = `waitpid` + `WEXITSTATUS`; the [syscall](../../../../glossary/terms.md#syscall) wrappers |
| 7 | `fork()` | trap to kernel: copy PCB + mark pages copy-on-write; returns twice (parent=child pid, child=0) |
| 8 | `pid < 0` | fork can fail (too many procs); always handle, like `malloc` NULL |
| 9–13 | child branch | `pid==0` means "I am the copy"; `execlp` replaces my memory with `/bin/echo`; `_exit` (not `exit`) avoids flushing parent's stdio twice |
| 14–16 | `waitpid` + print | parent blocks until child dies; `WEXITSTATUS` unpacks the exit byte the kernel saved in the PCB |

Change X → Y: change `"hello-from-child"` to `"hello-pid-test"` then `make run`. Verify: child line prints your new string (proves the child really exec'd a fresh program, not just printing from shared memory).

## Build It (QEMU link — what the kernel must do)

You don't reimplement fork in hosted C here; you list what `myos` will need (Phase 10). Read-only in this lesson:

```c
// kernel/proc.h (preview, not compiled here)
typedef struct pcb {
    int pid;          // index-card number
    int state;        // RUNNABLE, RUNNING, ZOMBIE
    void *pagetable;  // address-space root (see Memory phases)
    void *trapframe;  // saved registers for context switch
} pcb_t;
```

What this does: names the four fields every fork must copy and every switch must save.

| Lines | Code | Why it exists |
|---|---|---|
| `pid` | identity | parent/child distinguish via return value derived from this |
| `state` | lifecycle | ZOMBIE = dead but not yet `wait()`ed — why `wait` exists |
| `pagetable` | memory | fork copies the mapping (COW); exec replaces it |
| `trapframe` | CPU | context switch saves/restores registers here (next lesson) |

Change X → Y: add `int exitcode;` to the struct mentally → that's where `WEXITSTATUS` comes from. Verify: `grep -rn "struct proc" --include=*.h` in xv6 later will show the real version.

## Use It (Linux)

Trace the trio you just ran:

```bash
strace -f -e trace=process ./build/forkdemo
ps -o pid,ppid,stat,cmd -p $$ --ppid $$
```

What this does: shows the real `clone/fork + execve + wait` traps, then shows your shell's own process tree.

| Lines | Code | Why it exists |
|---|---|---|
| `strace -f -e trace=process` | follow forks, show process syscalls only | `-f` = trace children too; filter keeps `clone, execve, wait4, exit` visible |
| `./build/forkdemo` | your sim | generates the exact traps to study |
| `ps -o pid,ppid...` | tree snapshot | `ppid` = parent pid — the link `fork` created, `wait` reaps |

Change X → Y: replace `-e trace=process` with `-e trace=%process` (newer syntax) or drop the filter to see `openat/mmap` noise. Verify: with filter you see ~5 lines; without, hundreds — the OS does far more per exec than fork/exec/wait alone.

## Ship It

Artifact: `outputs/runbook-fork.md` — "zombie triage": if `ps` shows `Z`, parent forgot `wait`; if `fork` returns -1, check `ulimit -u` / dmesg. Reuse whenever Phase 10 clones its first child.

## Exercises

1. Easy — print `getpid()` and `getppid()` in both branches. Draw which pid is which.
2. Medium — make the child exit `42` (`_exit(42)`, no exec) and read it via `WEXITSTATUS` in the parent.
3. Hard — fork twice (parent → child → grandchild), have each print its pid/ppid, then `wait` in a chain so no zombies remain (preview of init reaping).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| PCB | kernel index card per process | [pcb](../../../../glossary/terms.md#pcb) |
| syscall | user→kernel trap (`fork`, `execve`, `wait4`) | [syscall](../../../../glossary/terms.md#syscall) |
| zombie | dead child not yet waited on | [pcb](../../../../glossary/terms.md#pcb) |
| copy-on-write | share pages until someone writes, then copy | [page](../../../../glossary/terms.md#page) |

## Further Reading

- OSTEP Ch.4–5 — the process + API story this lesson mirrors (read the fork/wait examples).
- `man 2 fork,execve,waitpid` — exact return/exit contracts we relied on.
- xv6 `kernel/proc.c:fork` — 20 lines that do what we described (read after this lesson).

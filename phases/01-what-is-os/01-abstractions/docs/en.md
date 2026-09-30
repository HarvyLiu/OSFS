# Abstractions — Files, Procs, Address Spaces

> The OS lies usefully: disk blocks become files, CPUs become processes, RAM becomes address spaces.

**Type:** Learn
**Languages:** C
**Prerequisites:** 01-wsl-docker-setup
**College ref:** OSTEP Ch.1–2 (three abstractions + dialogue), MIT 6.1810 Lec 1 (OS organization), xv6 file kernel/fs.h + kernel/proc.h (both cards previewed)
**Time:** ~45 minutes

## Learning Objectives
- Name the three lies (files, processes, address spaces) and what each hides
- Implement file write/read through fds and print your own address-space pieces
- Explain fds as per-process indices and pids as system-wide names
- Connect each abstraction to its phase (FS 07–08, Processes 02–04, Memory 05–06)

## Concept in 60s

![three abstractions](../figures/abstractions.svg)

<!-- source: ../figures/abstractions.excalidraw — open in excalidraw.com to redraw -->

Disks store numbered blocks (no names, no appends). The OS sells *files*: named byte streams with offsets (FS phases). CPUs run instruction streams; the OS sells *processes*: private machines with memory + files + a pid (Processes phases). RAM is one shared array; the OS sells *address spaces*: every process its own private mailboxes 0…max (Memory phases). Your program below touches all three in 20 lines: an fd (file handle), its own pid-less self (`/proc/self` later), and pointers into its private space. [Syscalls](../../glossary/terms.md#syscall) are the shop counter where you request each lie.

## Simulate It (host C — all three lies, portable, no QEMU)

Full program: `code/demo.c`. Writes a file, reads it back, prints where *it* lives.

```c
#include <stdio.h>
#include <string.h>

int main(void) {
    const char *path = "build/hello-osfs.txt";
    FILE *f = fopen(path, "w");
    if (!f) { perror("fopen w"); return 1; }
    fprintf(f, "hello from an abstraction\n");
    fclose(f);

    char buf[64];
    FILE *r = fopen(path, "r");
    if (!r) { perror("fopen r"); return 1; }
    if (!fgets(buf, sizeof buf, r)) { fprintf(stderr, "empty?\n"); return 1; }
    fclose(r);

    int stack_var = 1;
    printf("read back: %s", buf);
    printf("file=%s stack~%p code~%p\n", path, (void *)&stack_var, (void *)main);
    return strcmp(buf, "hello from an abstraction\n") != 0;
}
```

What this does: buys the file lie (named stream round-trip), then exhibits its own address space (stack vs code addresses side by side).

| Lines | Code | Why it exists |
|---|---|---|
| 5–8 | `fopen w` + `fprintf` + `fclose` | file as stream: open-by-name, write-at-offset-0, close-flushes (blocks+inodes underneath, FS phases) |
| 10–14 | `fopen r` + `fgets` + `fclose` | same name, fresh handle, independent offset (fds track position *per open*, not per file — the fact behind `fork`+file bugs later) |
| 16 | `stack_var` | one [stack](../../glossary/terms.md#stack) resident: its [address](../../glossary/terms.md#address) vs `main`'s shows the space's spread |
| 17–18 | two prints | content proof + layout proof (compare hex: stack high, code low — P-02/06's map, live) |
| 19 | `strcmp != 0` | exit code = round-trip fidelity (parents/CI read it — Processes-phase contract) |

Change X → Y: change the message string in *one* of the two places (write vs compare). Verify: exit 1 (proves the check compares, not echoes — fidelity is tested, not assumed).

## Build It

```bash
make run
ls -la build/hello-osfs.txt && xxd build/hello-osfs.txt | head -2
```

What this does: runs the round-trip, then inspects the artifact as bytes on disk (the lie lifted: names → blocks).

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | expect `read back: hello...` + two far-apart addresses |
| `ls -la` | metadata peek | size 26 (25 chars + newline): the file's *data* length, distinct from block allocation (FS phases quantify the gap) |
| `xxd` | byte truth | hex + ASCII side by side (P-00 reading skills, production file) |

Change X → Y: `echo EXTRA >> build/hello-osfs.txt`, rerun. Verify: program still prints original first line, `ls` size grows (proves opens don't truncate without `"w"` — modes matter; `"a"` appends, `"w"` razes).

## Use It (Linux)

Meet your process from outside itself (on a patient subject — the demo exits too fast to inspect, so `sleep` stands in; same kernel objects):

```bash
sleep 30 & MYPID=$!
ls -l /proc/$MYPID/fd/ | head -6
cat /proc/$MYPID/maps | head -4
kill $MYPID; wait $MYPID 2>/dev/null; echo reaped
```

What this does: starts a sleeping process, inspects its fds + maps, cleans up — process-as-object, observed externally.

| Lines | Code | Why it exists |
|---|---|---|
| `sleep 30 &` | background it | `&` = new process, shell keeps going (job control = user-level process API preview); `sleep` holds still while you look (observability needs stillness — debuggers freeze for the same reason) |
| `/proc/$PID/fd/` | open files live | `0→stdin 1→stdout 2→stderr` always (the first three fds — convention every `fork` inherits) |
| `/proc/$PID/maps` | space live | stack/heap/code ranges of *another* process (your pointers are numbers in a space like this) |
| `kill/wait` | tidy up | `kill` defaults SIGTERM (polite); `wait` reaps (no zombies — 02-fork-exec's rule, honored) |

Change X → Y: replace `sleep 30` with `sleep 0.1`. Verify: `ls: cannot access /proc/...` sometimes (the race made visible — exited before inspection; *this* is why `wait` + zombies exist).

## Ship It

Artifact: `outputs/lies-card.md` — file/proc/space one-liners (hides/what/phase), fd 0/1/2 rule, `/proc` verbs. The course map in a pocket: every phase deepens exactly one lie.

## Exercises

1. Easy — print `fileno(f)` after `fopen w` (expect 3: 0/1/2 taken — fds are per-process indices from 3 up).
2. Medium — open the same path twice, write different lines via each handle, close both, `cat` the file (interleaved offsets? Last-close-wins? Record the bytes — FS-phase preview of offset semantics).
3. Hard — `fork()` then both branches append to the file (Linux-only, after 02-fork-exec): whose bytes land where? (Shared offset vs separate opens — the answer splits by `O_APPEND`; find out empirically.)

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| syscall | open/read/write/fork: the counter where lies are requested | ../../glossary/terms.md#syscall |
| address | private per process (same number, different bytes across procs) | ../../glossary/terms.md#address |
| heap | per-process region (malloc bytes invisible to siblings) | ../../glossary/terms.md#heap |
| PCB | kernel's per-process card (pid, fds, address-space root) | ../../glossary/terms.md#pcb |

## Further Reading

- OSTEP Ch.1–2 — the dialogue this lesson stages (read the full 10-page version now; it will feel easy).
- `man 2 open,read,write,close` — fd semantics behind every `FILE*`.
- `man 5 proc` — `/proc/PID/{fd,maps,status}` field guides for the tour above.

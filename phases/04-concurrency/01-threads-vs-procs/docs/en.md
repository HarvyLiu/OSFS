# Threads vs Processes, Races — Sharing Is the Bug Factory

> Processes share nothing (clone the world). Threads share everything (argue over it).

**Type:** Build
**Languages:** C
**Prerequisites:** 03-cfs-intuition
**College ref:** OSTEP Ch.26–27 (threads API + races), MIT 6.1810 thread switching notes, xv6 file kernel/proc.c (`clone`-shaped growth preview)
**Time:** ~60 minutes

## Learning Objectives
- Trace one address space with N stacks (threads) vs N address spaces (processes) using a memory diagram
- Implement a racy counter and a mutex-fixed counter with pthreads, reading the shortfall off
- Explain `counter++` as load-add-store and why interleavings lose updates
- Connect threads to scheduler (same CFS, shared PCB-adjacent state) and to locks (next lesson's fix, generalized)

## Concept in 60s

![threads vs procs](../figures/threads-procs.svg)

<!-- source: ../figures/threads-procs.excalidraw — open in excalidraw.com to redraw -->

Sit beside a program that counts to 400,000 and gets the answer wrong. That is your starting scene. `fork()` clones code+data+files into separate worlds — talking needs pipes. `pthread_create` adds a [stack](../../../../glossary/terms.md#stack) + [register](../../../../glossary/terms.md#register) set to the *same* world, so all globals come shared for free — and for danger. `counter++` compiles to load→add→store: two threads can load the same value, both add, both store — and one update vanishes. Four threads × 100k increments *should* be 400k; racy builds land short (how short depends on cores and timing — nondeterminism is the symptom). A mutex serializes the triple into atomicity. You use it here as the control group; next lesson derives *how* it works.

## Simulate It (host pthreads — Linux/WSL/Docker; MinGW also provides pthreads)

Your specimen is `code/race.c`. You run the same work twice, under two disciplines.

```c
#include <stdio.h>
#include <pthread.h>

#define NTHREAD 4
#define NINCR 100000

static long counter = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void *racy(void *arg) {
    (void)arg;
    for (int i = 0; i < NINCR; i++) counter++;
    return 0;
}

static void *safe(void *arg) {
    (void)arg;
    for (int i = 0; i < NINCR; i++) {
        pthread_mutex_lock(&lock);
        counter++;
        pthread_mutex_unlock(&lock);
    }
    return 0;
}

static long run(void *(*fn)(void *)) {
    pthread_t ts[NTHREAD];
    counter = 0;
    for (int i = 0; i < NTHREAD; i++) pthread_create(&ts[i], 0, fn, 0);
    for (int i = 0; i < NTHREAD; i++) pthread_join(ts[i], 0);
    return counter;
}

int main(void) {
    long want = (long)NTHREAD * NINCR;
    long racy_got = run(racy);
    long safe_got = run(safe);
    printf("no-mutex: got=%ld want=%ld %s\n", racy_got, want,
           racy_got == want ? "(exact this run: lucky timing)" : "(short: lost updates)");
    printf("mutex: got=%ld want=%ld\n", safe_got, want);
    return safe_got != want;
}
```

What this does: runs 400k increments racy (usually short) then locked (always exact) — the before/after that justifies all of synchronization.

| Lines | Code | Why it exists |
|---|---|---|
| 3–4 | `NTHREAD/NINCR` | 400k total: big enough to race on multicore, fast enough (<1s) for CI |
| 6–7 | shared `counter` + static lock | `static` = one instance, zero-init = unlocked mutex (the sharing under test) |
| 9–13 | `racy` | bare `counter++`: load-add-store, interruptible between any two (the 3-instruction window) |
| 15–23 | `safe` | same work fenced: lock→triple→unlock (control group proving the hardware *can* count) |
| 25–32 | `run` | create-join harness: `pthread_create` (new stack+regs, same address space) then `pthread_join` (wait's thread cousin) |
| 36–40 | report both | racy line informational (timing-dependent!), mutex line contractual (exit code gates on it only) |

Change X → Y: change `NINCR` to `1000`. Verify: racy often prints exact (window too small to collide — proves races are *probabilistic*: small overlap hides them, scale exposes them; testing must force interleavings, not hope).

## Build It

```bash
make run
make test
```

What this does: builds with `-pthread` (both compile + link need it), runs both disciplines, asserts the mutex line exact.

| Lines | Code | Why it exists |
|---|---|---|
| `-pthread` | everywhere | defines `_REENTRANT`, links `libpthread`: forget the link half and `pthread_create` goes undefined (the classic two-half flag) |
| `make test` | gates on mutex only | racy outcome is timing — asserting it exact would flake CI (assert the deterministic control, observe the wild) |

Change X → Y: drop `-pthread` from the link line only. Verify: `undefined reference to pthread_create` (proves the flag has compile *and* link halves — one without the other fails differently each way).

## Use It (Linux)

You can see your threads from outside as tasked clones sharing memory:

```bash
./build/race | head -3
ps -eLf | head -5
cat /proc/self/status | grep -E "Threads|VmRSS"
```

What this does: runs your race, lists system threads (`L` = thread view), and shows your shell's own thread count + footprint.

| Lines | Code | Why it exists |
|---|---|---|
| `ps -eLf` | every thread, every proc | `LWP` column = thread ids sharing one `PID` (your 4 workers would show as 4 LWPs under 1 PID mid-run) |
| `/proc/self/status` | self census | `Threads: 1` for `cat` itself (single-threaded baseline — threaded servers show N) |

Change X → Y: `ps -eLf | wc -l` idle vs while `make run` spins in another terminal. Verify: count bumps during the run (threads are countable kernel objects, like processes with shared guts).

## Ship It

Artifact: `outputs/race-card.md` — sharing checklist (what's shared: globals/heap/files; what's private: stack/registers/errno), race triage (flaky count → suspect shared write; exact under small N → scale up), `-pthread` both-halves reminder. Reuse in every later "is it a race?" investigation. You now own the bug — locks fence it next.

## Exercises

1. Easy — sweep `NINCR` in {100, 1k, 10k, 100k}, record racy shortfall each (3 runs). Plot shortfall vs work (races scale with collision windows, not linearly — see it once, believe forever).
2. Medium — protect with one lock per *half* the threads (two mutexes, even/odd). Still exact? (Yes — disjoint data... but same counter! Show it *fails*: locks only work when *all* accessors share *one* — the discipline lesson locks-next formalizes.)
3. Hard — replace `counter++` with `__atomic_fetch_add(&counter, 1, __ATOMIC_SEQ_CST)` (P-02/05 inline-ASM territory: one uninterruptible instruction). Show exact *without* mutex, then argue when atomics suffice vs full locks (single op vs critical *sections*).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| stack | per-thread private (N threads = N stacks, 1 address space) | [stack](../../../../glossary/terms.md#stack) |
| register | per-thread live state (switch saves/restores per thread, same as 02/03) | [register](../../../../glossary/terms.md#register) |
| heap | shared by default across threads (the bug factory + the feature) | [heap](../../../../glossary/terms.md#heap) |
| syscall | `clone` creates threads; `futex` sleeps mutexes (next lesson's floor) | [syscall](../../../../glossary/terms.md#syscall) |

## Further Reading

- OSTEP Ch.26–27 — API + the lost-update derivation this lesson runs.
- `man 7 pthreads` + `man 3 pthread_create` — sharing table (what's shared/private, authoritative).
- `man 2 clone` — the one syscall behind both fork and thread creation (flags pick the sharing).

# Semaphores, Condvars, Deadlock — Sleeping Instead of Spinning

> Locks fence. Semaphores count. Condvars wait *for a reason*. Deadlock waits forever.

**Type:** Build
**Languages:** C
**Prerequisites:** 02-ticket-spin
**College ref:** OSTEP Ch.30–32 (sems/condvars/deadlock), MIT 6.1810 sleep/wakeup notes, xv6 file kernel/proc.c (`sleep`/`wakeup` channel preview)
**Time:** ~90 minutes

## Learning Objectives
- Trace a bounded buffer (empty/full counts + mutex) through 200 items without loss or spin
- Implement a counting semaphore from mutex+cond and use two of them to pace producer/consumer
- Explain `while`-not-`if` around `wait` (spurious + stolen wakeups) and lock-ordering vs deadlock
- Connect sleep locks to ticket spin (same exclusion, surrendered CPU) and to `futex` (next: kernel-assisted sleep)

## Concept in 60s

![bounded buffer](../figures/bounded-buffer.svg)

<!-- source: ../figures/bounded-buffer.excalidraw — open in excalidraw.com to redraw -->

A semaphore is a *count* with atomic sleep: `wait` blocks while 0, `post` wakes one. Bounded buffer needs two: `empty` (starts N=slots, producer waits/posts) and `full` (starts 0, consumer waits/posts), plus a mutex for the indices (counts pace, mutex protects). A condvar is dumber + sharper: `wait(mutex, cond)` sleeps *until signaled for a reason* — always recheck in `while` (wakeups can be spurious or stolen). Deadlock = circular wait (A→B vs B→A): four Coffman conditions, one practical cure — global lock order (always A-then-B, in *every* path).

## Simulate It (host pthreads — portable hand-rolled semaphores, no `sem.h`)

Three files: `code/sem.h` + `code/sem.c` (counter from mutex+cond) + `code/pc.c` (200-item pipeline).

```c
// sem.h -- counting semaphore shape.
#ifndef OSFS_SEM_H
#define OSFS_SEM_H

#include <pthread.h>

typedef struct { int count; pthread_mutex_t m; pthread_cond_t cv; } csem_t;
void csem_init(csem_t *s, int v);
void csem_wait(csem_t *s);
void csem_post(csem_t *s);

#endif
```

What this does: publishes count+sleep-queue as one object — `wait`/`post` are the only verbs (counts never touched directly, like tickets but sleeping).

```c
// sem.c -- sleep instead of spin.
#include "sem.h"

void csem_init(csem_t *s, int v) {
    s->count = v;
    pthread_mutex_init(&s->m, 0);
    pthread_cond_init(&s->cv, 0);
}

void csem_wait(csem_t *s) {
    pthread_mutex_lock(&s->m);
    while (s->count == 0)
        pthread_cond_wait(&s->cv, &s->m);
    s->count--;
    pthread_mutex_unlock(&s->m);
}

void csem_post(csem_t *s) {
    pthread_mutex_lock(&s->m);
    s->count++;
    pthread_cond_signal(&s->cv);
    pthread_mutex_unlock(&s->m);
}
```

What this does: blocks instead of burning — zero-count waiters surrender the CPU until a post wakes one (04/02's spin, civilized).

| Lines | Code | Why it exists |
|---|---|---|
| 5–9 | init | count seeds capacity (`N` empty slots / `0` full ones); mutex+cond start fresh (destroy omitted — process-lifetime objects, noted) |
| 13–14 | `while(count==0) wait` | `while` not `if`: spurious wakeups (allowed by spec!) and stolen signals (another consumer grabbed it first) both re-sleep correctly; `if` would proceed on lies |
| 15–16 | decrement + unlock | take one unit of resource *inside* the fence (count and queue move atomically — the invariant) |
| 20–24 | post+signal | add one, wake one waiter (`signal` = one, `broadcast` = all — one suffices since one unit arrived) |

Change X → Y: change `while` to `if`, hammer with 4 producers × 4 consumers (Exercise 2 setup). Verify: occasional negative counts / lost items under contention (the `if` bug needs pressure to show — which is why the rule is unconditional).

```c
// pc.c -- 200 items, buffer 8, zero loss.
#include <stdio.h>
#include <pthread.h>
#include "sem.h"

#define NBUF 8
#define NITEM 200

static int buf[NBUF], in = 0, out = 0;
static pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;
static csem_t empty, full;

static void *producer(void *arg) {
    (void)arg;
    for (int i = 0; i < NITEM; i++) {
        csem_wait(&empty);
        pthread_mutex_lock(&mx);
        buf[in] = i;
        in = (in + 1) % NBUF;
        pthread_mutex_unlock(&mx);
        csem_post(&full);
    }
    return 0;
}

static void *consumer(void *arg) {
    long *sum = arg;
    for (int i = 0; i < NITEM; i++) {
        int v;
        csem_wait(&full);
        pthread_mutex_lock(&mx);
        v = buf[out];
        out = (out + 1) % NBUF;
        pthread_mutex_unlock(&mx);
        csem_post(&empty);
        *sum += v;
    }
    return 0;
}

int main(void) {
    pthread_t p, c;
    long sum = 0;
    csem_init(&empty, NBUF);
    csem_init(&full, 0);
    pthread_create(&p, 0, producer, 0);
    pthread_create(&c, 0, consumer, &sum);
    pthread_join(p, 0);
    pthread_join(c, 0);
    long want = (long)NITEM * (NITEM - 1) / 2;
    printf("consumed sum=%ld want=%ld\n", sum, want);
    return sum != want;
}
```

What this does: paces 200 integers through 8 slots — producers sleep when full, consumers sleep when empty, checksum proves zero loss/reorder-effect (sum is order-free by design).

| Lines | Code | Why it exists |
|---|---|---|
| 9–10 | indices + mutex | `in`/`out` ring modulo `NBUF`; `mx` guards *positions* (semaphores pace *counts* — two jobs, two tools) |
| 15–22 | produce order | wait-empty → place → post-full (never touch `buf` outside empty-ticket — the pacing contract) |
| 26–36 | consume order | wait-full → take → post-empty, sum outside the lock (hold fences for indices only — throughput lives in short sections) |
| 43–44 | checksum | 0+…+199 = 19900 regardless of interleave (order-free verification: assert the invariant, not the schedule) |

Change X → Y: change `NBUF` to `1`. Verify: still exact, slower (`time` it — throughput vs buffer size, measured; the pipeline-depth knob made numeric).

Deadlock, safely demonstrated (`code/order.c` — trylock probe, never hangs):

```c
#include <stdio.h>
#include <pthread.h>

static pthread_mutex_t A = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t B = PTHREAD_MUTEX_INITIALIZER;

static int consistent(void) { // always A-then-B: composes with itself
    pthread_mutex_lock(&A);
    pthread_mutex_lock(&B);
    pthread_mutex_unlock(&B);
    pthread_mutex_unlock(&A);
    return 0;
}

int main(void) {
    // Inverted attempt would block; trylock observes instead of hanging:
    pthread_mutex_lock(&B);
    int took_a = (pthread_mutex_trylock(&A) == 0);
    if (took_a) pthread_mutex_unlock(&A);
    pthread_mutex_unlock(&B);
    consistent();
    printf("order-rule: consistent path ok; inverted try took_a=%d (single-thread: 1; under contention: races toward deadlock)\n", took_a);
    return 0;
}
```

What this does: shows the cure (global order, always composable) beside a *probe* of the disease (inverted try under contention races toward circular wait — observed, never hung).

| Lines | Code | Why it exists |
|---|---|---|
| 6–11 | consistent | A-then-B everywhere = no cycle possible (Coffman broken at "circular wait" — the cheapest condition to kill) |
| 15–18 | trylock probe | `trylock` = ask-without-sleeping (lock *query* — deadlock study without deadlock participation) |

Change X → Y: run two `order` processes under `taskset -c 0` peppered with loops (Exercise 3). Verify discussion: single-threaded `took_a=1` always (the probe needs real contention to bite — deadlocks are emergent, which is why order *discipline* beats testing).

## Build It

```bash
make run
make test
```

What this does: runs pipeline (expect `sum=19900`) + order probe, then asserts sum, semaphore unit semantics, and trylock self-consistency.

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | `consumed sum=19900 want=19900` + order line (both disciplines, one command) |
| `make test` | machine proof | threaded pc at small scale (exact) + single-thread sem countdown/post + consistent-order pair |

Change X → Y: seed `full` with `NBUF` instead of `0`. Verify: consumer reads garbage first (counts lie → protocol lies — initialization *is* the protocol).

## Use It (Linux)

Sleeping primitives leave different footprints than spin:

```bash
time ./build/pc
ps -eo stat,comm | sort | uniq -c | sort -rn | head -5
```

What this does: times the pipeline (mostly *waiting*, not burning), then censuses process states (`S` sleepers dominate healthy systems — spinners show as `R`).

| Lines | Code | Why it exists |
|---|---|---|
| `time` | cost shape | user ≪ wall (surrendered CPU — compare 04/02's user≈wall×cores spin signature) |
| `ps stat` | state census | `S` = sleeping (our waiters' public face), `R` = running/spinning, `D` = uninterruptible (disk — Persistence phases' native state) |

Change X → Y: `time` the ticket demo beside this. Verify: ticket user-time higher per unit work (spin bills cores; sleep bills latency — the cost-model choice, numeric).

## Ship It

Artifact: `outputs/sync-card.md` — sem/wait/post, empty+full+mutex trio recipe, `while`-not-`if`, global-order rule, spin-vs-sleep costs. The synchronization pocket reference through capstone.

## Exercises

1. Easy — NBUF 8→2→1, `time` each (report wall+user; explain why correctness holds at all three while speed doesn't).
2. Medium — 2 producers × 2 consumers × 100 items with `while`→`if` sabotage (count how many runs in 20 go wrong — flake-rate statistics, the honest way to fear `if`).
3. Hard — genuine 2-thread AB-BA deadlock in a *child process* with parent `timeout`+kill and exit-code report (deadlock observed scientifically: contained, timed, reaped — never hang your shell for science).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| heap | buffer + semaphores shared (fenced, paced — the civilized factory) | ../../glossary/terms.md#heap |
| stack | per-thread; waiters' stacks persist while descheduled (sleep ≠ death) | ../../glossary/terms.md#stack |
| syscall | `futex` underpins real sleep locks (this lesson's pure-userspace prequel) | ../../glossary/terms.md#syscall |
| PCB | blocked state lives here (RUNNABLE→SLEEPING + wakeup channel) | ../../glossary/terms.md#pcb |

## Further Reading

- OSTEP Ch.30–32 — semaphores, condvars (Mesa vs Hoare), deadlock+Coffman (the trilogy, full).
- `man 3 pthread_cond_wait` — the `while` rule in the spec's own words (read twice).
- xv6 `kernel/proc.c:sleep/wakeup` — channel sleep: counts replaced by addresses, same idea.

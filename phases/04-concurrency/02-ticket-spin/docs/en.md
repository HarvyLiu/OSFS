# Ticket + Spin Locks — Fairness From Two Integers

> Take a number. Wait your turn. The numbers never lie, and nobody cuts.

**Type:** Build
**Languages:** C
**Prerequisites:** 01-threads-vs-procs
**College ref:** OSTEP Ch.28 (locks: spin, ticket, futex preview), MIT 6.1810 `spinlock.h` (the real two-field lock), xv6 file kernel/spinlock.c (acquire/release yours mirrors)
**Time:** ~90 minutes

## Learning Objectives
- Trace `fetch_add` tickets vs `now_serving` display through 4 threads' interleaving
- Implement a ticket lock with C11 atomics and prove exact counts + FIFO admission
- Explain spin cost (burned CPU) vs sleep cost (context switch) and when each wins
- Connect `atomic_fetch_add` to P-02/05's single-instruction atomicity (hardware keeps the promise)

## Concept in 60s

![ticket queue](../figures/ticket-queue.svg)

<!-- source: ../figures/ticket-queue.excalidraw — open in excalidraw.com to redraw -->

Two shared integers: `next_ticket` (deli roll) and `now_serving` (display). Lock = `my = fetch_add(&next,1)` (atomically take-and-increment — one uninterruptible op, P-02/05's territory), then spin `while (now_serving != my)` (reread each lap — `atomic_load`, never cache). Unlock = `now_serving++` (next number, please). Admission order = ticket order = arrival order: FIFO fairness, no starvation, starvation-free by construction. Cost: spinners burn cores (fine for 10-instruction critical sections, insane for disk waits — sleep locks/futex later).

## Simulate It (host pthreads + C11 atomics — Linux/WSL/Docker/MinGW)

Split across `code/ticket.h` + `code/ticket.c` + `code/demo.c` (headers/split practice, again on purpose).

```c
// ticket.h -- two integers + three verbs.
#ifndef OSFS_TICKET_H
#define OSFS_TICKET_H

typedef struct { _Atomic unsigned next_ticket; _Atomic unsigned now_serving; } ticket_t;
void ticket_init(ticket_t *l);
void ticket_lock(ticket_t *l);
void ticket_unlock(ticket_t *l);

#endif
```

What this does: publishes the deli-counter shape — `_Atomic` forbids the load-add-store split that lost 285k updates in 04/01.

| Lines | Code | Why it exists |
|---|---|---|
| 5 | `_Atomic` both fields | every access is one indivisible hardware step (x86 `lock xadd` underneath — P-02/05's atomicity, C-spelled) |
| 6–8 | init/lock/unlock | the whole API: reset, take-number-and-wait, advance-display (compare pthread's dozen calls — same power, visible gears) |

```c
// ticket.c -- fairness in 10 lines.
#include <stdatomic.h>
#include "ticket.h"

void ticket_init(ticket_t *l) {
    atomic_store(&l->next_ticket, 0);
    atomic_store(&l->now_serving, 0);
}

void ticket_lock(ticket_t *l) {
    unsigned my = atomic_fetch_add(&l->next_ticket, 1);
    while (atomic_load(&l->now_serving) != my) {
#if defined(__x86_64__) || defined(_M_X64)
        __asm__ volatile ("pause");
#endif
    }
}

void ticket_unlock(ticket_t *l) {
    atomic_fetch_add(&l->now_serving, 1);
}
```

What this does: numbers arrivals atomically, parks each on its number, hands off by increment — FIFO admission with starvation impossible (your number always comes).

| Lines | Code | Why it exists |
|---|---|---|
| 5–8 | init stores | release-semantics reset (plain `=` would also work pre-sharing; atomics document intent from line one) |
| 11 | `fetch_add` take | THE atomic step: read-and-bump as one (two threads can never draw the same number — hardware serializes) |
| 12–16 | spin on load | reread `now_serving` every lap (`atomic_load` blocks hoisting into a [register](../../glossary/terms.md#register) — caching it would spin forever); `pause` hints the core (spin politely: power + sibling-hyperthread throughput) |
| 20–22 | unlock bump | display advances; exactly one waiter matches and proceeds (no thundering herd — only the holder writes) |

Change X → Y: delete the `pause` lines. Verify: still exact, slightly hotter under contention (measure with `time`: user-time climbs — politeness has a meter, impoliteness too).

```c
// demo.c -- 400k increments under tickets.
#include <stdio.h>
#include <pthread.h>
#include "ticket.h"

#define NTHREAD 4
#define NINCR 100000

static long counter = 0;
static ticket_t lock;

static void *worker(void *arg) {
    (void)arg;
    for (int i = 0; i < NINCR; i++) {
        ticket_lock(&lock);
        counter++;
        ticket_unlock(&lock);
    }
    return 0;
}

int main(void) {
    pthread_t ts[NTHREAD];
    ticket_init(&lock);
    for (int i = 0; i < NTHREAD; i++) pthread_create(&ts[i], 0, worker, 0);
    for (int i = 0; i < NTHREAD; i++) pthread_join(ts[i], 0);
    long want = (long)NTHREAD * NINCR;
    printf("ticket: got=%ld want=%ld\n", counter, want);
    return counter != want;
}
```

What this does: repeats 04/01's workload with hand-rolled fairness — expect byte-identical exactness to the mutex run (400000), owned end-to-end.

| Lines | Code | Why it exists |
|---|---|---|
| 9–16 | fenced triple | lock→count→unlock: the critical *section* (vs Exercise-3's single-op atomics — sections need locks, ops may not) |
| 22–26 | harness | same create-join as 04/01 (compare files: only the fence changed — mechanism swap, workload fixed) |

Change X → Y: comment out lock+unlock (keep counter++). Verify: shortfall returns (~115k-ish on your box — the same wound, reopened on purpose, proving the lock was the medicine).

## Build It

```bash
make run
make test
```

What this does: builds `-pthread -std=c11` (atomics need C11), runs the exact count, asserts it plus single-thread + FIFO-order checks.

| Lines | Code | Why it exists |
|---|---|---|
| `-std=c11` | atomics language | `_Atomic`/`stdatomic.h` are C11 (older `-std` rejects the header — the flag is the feature) |

Change X → Y: build with `-std=c99`. Verify: `_Atomic` unknown (proves the feature's vintage — language versions gate hardware access).

## Use It (Linux)

Locks leave footprints in contention profiles:

```bash
./build/ticketdemo
perf stat -e cycles,instructions,cache-misses ./build/ticketdemo 2>&1 | head -8 || echo "(no perf: containers/WSL often lack counters; time instead)" && time ./build/ticketdemo
```

What this does: counts cycles/instructions/misses per 400k fenced increments (spin cost, numeric) or falls back to wall time.

| Lines | Code | Why it exists |
|---|---|---|
| `perf stat` | hardware truth | spinning shows as high instructions-per-increment vs mutex-sleep builds (different costs, visible here first, theorized in 04/03) |
| `time` fallback | dignity | containers deny counters; wall+user time still separates spin (user≈wall×cores) from sleep (user≪wall) |

Change X → Y: `perf stat` the 04/01 mutex binary beside this one. Verify: ticket burns more user cycles under contention (spinning is *honest* overhead — named, measured, later avoided with futex).

## Ship It

Artifact: `outputs/lock-card.md` — take/wait/advance trio, spin-vs-sleep rule (short+contended=spin, long+idle=sleep), `pause` note, `_Atomic` minimum (never hand-roll with plain ints — 04/01's 285k says why). Next lesson generalizes to sleep locks; this card stays the fairness reference.

## Exercises

1. Easy — record `time` for ticket vs mutex builds (3 runs each, report user/sys). Explain the gap (spin burns, futex sleeps).
2. Medium — add `ticket_trylock` (succeed iff `now_serving == next_ticket`, atomically). Prove with a single-thread test + a contention demo (try fails sometimes — that's the API's honesty).
3. Hard — implement test-and-set spinlock (one `locked` flag, `atomic_exchange`), hammer both with 8 threads, compare fairness: record per-thread *order* of first 8 acquisitions (ticket: round-robin-ish; TAS: whoever shouts loudest — starvation made visible, the reason tickets exist).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| register | `fetch_add` executes in hardware on cache lines backing these fields | ../../glossary/terms.md#register |
| heap | lock + counter live shared (same factory as 04/01's bug, now fenced) | ../../glossary/terms.md#heap |
| stack | each spinner has its own (spinning burns core, not stack — depth stays flat) | ../../glossary/terms.md#stack |
| syscall | none here (pure userspace spin); futex sleeps need the kernel (next) | ../../glossary/terms.md#syscall |

## Further Reading

- OSTEP Ch.28 — spin vs ticket vs futex cost model (this lesson runs the first two columns).
- xv6 `kernel/spinlock.c` — `acquire`/`release` with pushcli/popcli flavor (same trio + interrupt fencing).
- `man 3 pthread_mutex_lock` vs `man 2 futex` — sleep-side contracts for contrast.

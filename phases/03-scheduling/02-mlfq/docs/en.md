# MLFQ — Short Jobs First, Without Knowing the Future

> SJF cheats by knowing bursts. MLFQ learns them by watching: spend your quantum, drop a level.

**Type:** Build
**Languages:** C
**Prerequisites:** 01-fifo-sjf-rr
**College ref:** OSTEP Ch.8 (MLFQ rules + starvation/boost), MIT 6.1810 scheduling notes, xv6 file kernel/proc.c (single-queue RR you'll extend)
**Time:** ~90 minutes

## Learning Objectives
- Trace 3 queues (q=2/4/FCFS) on a fixed workload tick-by-tick, demotions included
- Implement a real MLFQ engine (demote on exhaust, FIFO within queue, periodic boost) in ~80 lines
- Explain starvation (hog camps low while shorts trickle high) and why boost bounds it
- Connect quantum choice to context-switch cost (02/03) and priority to niceness (Use It)

## Concept in 60s

![mlfq queues](../figures/mlfq-queues.svg)

<!-- source: ../figures/mlfq-queues.excalidraw — open in excalidraw.com to redraw -->

You cannot ask a job how long it will run, so you watch what it does. That is the whole trick here. Everyone starts at the top, in Q0 (quantum 2). Spend your quantum and you drop one level, to Q1 (quantum 4), then to Q2 (FCFS). Finish and you leave, and a higher non-empty queue always preempts a lower one. Short jobs finish high before hogs drag them down — SJF's effect learned, not foretold. The failure mode is honest: a steady hog sinks to Q2 and starves while shorts keep arriving. The fix is periodic boost — every N ticks, everyone returns to Q0 — which bounds waiting at the price of forgetting history. You will trace the same trio as 03/01: A(arr 0, burst 3), B(arr 0, burst 10), C(arr 0, burst 3).

## Simulate It (host — the engine, no QEMU)

Your engine lives in `code/mlfq_engine.c` (+`mlfq.h`). You pick the highest runnable job, run one unit, account for it, then demote or boost.

```c
#include "mlfq.h"

static const int QQUANTUM[3] = {2, 4, 1 << 30};

void mlfq_run(mjob_t *js, int n, int boost_every, int *order, int *norder) {
    int t = 0, ndone = 0, no = 0;
    for (int i = 0; i < n; i++) { js[i].rem = js[i].burst; js[i].q = 0; js[i].qu = 0; js[i].qs = 0; js[i].comp = 0; }
    while (ndone < n) {
        int pick = -1;
        for (int i = 0; i < n; i++) {
            if (js[i].arr > t || js[i].comp) continue;
            if (pick < 0 || js[i].q < js[pick].q ||
                (js[i].q == js[pick].q && (js[i].qs < js[pick].qs ||
                 (js[i].qs == js[pick].qs && js[i].id < js[pick].id))))
                pick = i;
        }
        mjob_t *j = &js[pick];
        j->rem--;
        j->qu++;
        t++;
        if (j->rem == 0) { j->comp = t; order[no++] = j->id; ndone++; }
        else if (j->qu >= QQUANTUM[j->q]) {
            if (j->q < 2) j->q++;
            j->qu = 0;
            j->qs = t;
        }
        if (boost_every > 0 && t % boost_every == 0) {
            for (int i = 0; i < n; i++)
                if (!js[i].comp) { js[i].q = 0; js[i].qu = 0; js[i].qs = t; }
        }
    }
    *norder = no;
}
```

What this does: advances one time unit per loop — pick (highest queue, FIFO within, pid tiebreak), charge, demote-or-complete, boost on schedule — until all complete.

| Lines | Code | Why it exists |
|---|---|---|
| 3 | `QQUANTUM` | Q2 = huge = FCFS in practice (never exhausts); quanta are *the* policy knobs |
| 6 | reset loop | engine re-runnable (main runs boost-off then boost-on on fresh copies) |
| 8–14 | pick | min queue wins; `qs` (queue-entry time) = FIFO within; `id` = deterministic tiebreak (no dict-order flakiness in tests) |
| 15–18 | charge + tick | 1 unit of CPU; `t` = global clock completions are stamped with |
| 19 | `comp = t` | finish time (turnaround = comp − arr, as in 03/01) |
| 20–24 | demote | quantum exhausted but alive → sink one level, reset usage, re-stamp `qs` (back of the new line) |
| 25–28 | boost | every `boost_every` ticks: all unfinished to Q0, usage cleared — starvation bound, history forgotten |

Change X → Y: change Q0 quantum `2` → `1`. Verify: `make run` completions shift (more demotions, shorts relatively favored — count the extra Q0 rounds in the log).

Driver (`code/main.c`):

```c
#include <stdio.h>
#include "mlfq.h"

static void show(const char *tag, int boost) {
    mjob_t js[3] = {{1,0,3,0,0,0,0,0},{2,0,10,0,0,0,0,0},{3,0,3,0,0,0,0,0}};
    int order[3], no = 0;
    mlfq_run(js, 3, boost, order, &no);
    printf("%s comp: A=%d B=%d C=%d order %d-%d-%d\n", tag,
           js[0].comp, js[1].comp, js[2].comp, order[0], order[1], order[2]);
}

int main(void) {
    show("NOBOOST", 0);
    show("BOOST8", 8);
    return 0;
}
```

What this does: runs the same trio twice — pure MLFQ vs 8-tick boost — printing completions + finish order for direct comparison.

| Lines | Code | Why it exists |
|---|---|---|
| 5 | fixed trio | A=short, B=hog, C=short: the minimal cast that shows demotion *and* boost effects |
| 6 | engine call | `boost=0` disables (the `>0` guard); same binary, one flag apart |
| 10–13 | both modes | expect `NOBOOST ... A=7 B=16 C=12` and `BOOST8 ... A=7 B=16 C=11` (hand-traced below) |

Change X → Y: change boost `8` → `4`. Verify: C finishes even earlier relative to B (more forgetting helps shorts; hogs pay — the tradeoff, numeric).

Hand trace (no-boost — every constant's alibi):

```text
t0-2  A Q0 (rem3->1, exhaust ->Q1)   t2-4  B Q0 (rem10->8 ->Q1)
t4-6  C Q0 (rem3->1 ->Q1)             t6-7  A Q1 (rem1 ->DONE@7)
t7-11 B Q1 q4 (rem8->4, exhaust ->Q2) t11-12 C Q1 (rem1 ->DONE@12)
t12-16 B Q2 FCFS (rem4 ->DONE@16)
```

What this does: proves `A=7 B=16 C=12` interval by interval — demotions, FIFO-within-queue (A,B,C order into Q1), and the FCFS tail.

Boost-8 trace (diff only): ticks 0–7 identical (A done@7, B mid-Q1-slice with 1 unit used); at t=8 boost returns B,C to Q0; B runs t8–10 (exhaust →Q1), C runs t10–11 (DONE@11); B Q1 t11–15 (exhaust →Q2), done t16. So `A=7 B=16 C=11` — boost buys C one tick here; with a *stream* of shorts it buys the hog its life (starvation story in Exercises).

## Build It

```bash
make run
make test
```

What this does: prints both modes, then asserts completions + order for both (6+ checks on the real engine, not copies).

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | two lines, diff = boost's footprint |
| `make test` | machine proof | greps both lines + runs `test_main` against `mlfq_engine.c` directly |

Change X → Y: set B burst 10→20 in `main.c` only (not tests). Verify: test fails on B's completion (proves tests pin the workload — update trace + tests together, never one).

## Use It (Linux)

Feel quanta and boosts in the wild:

```bash
chrt -p $$ 2>/dev/null || taskset -p $$
ps -eo pid,ni,pri,stat,cmd | sort -k3 -n | head -6
```

What this does: shows your shell's policy/priority, then the machine's most-favored processes by kernel priority — CFS's dynamic levels, MLFQ's industrial cousin.

| Lines | Code | Why it exists |
|---|---|---|
| `chrt/taskset` | affinity + policy | which CPUs you may run on + under which `SCHED_*`; caged runners feel like permanent Q2 |
| `ps ... pri` | live levels | low `PRI` numbers = high favor (inverted scale — read carefully); `NI` feeds the weight math |

Change X → Y: `nice -n 10 sleep 30 & ps -o pid,ni,pri,comm -p $!` then kill it. Verify: `NI=10` maps to worse `PRI` (user hint flows into placement — boost's polite inverse).

## Ship It

Artifact: `outputs/mlfq-card.md` — the 4 rules + quantum guidance (small top for response, big bottom for throughput, boost ~50–100ms real systems) + starvation symptoms (`Q2 age` growing in your future telemetry). Cite when justifying any priority scheme to capstone reviewers. You now own learned priorities — CFS prices them differently next.

## Exercises

1. Easy — re-trace with A burst 3→1. Show A never leaves Q0 and completes at t=1 (shorts fly high — the design goal, numeric).
2. Medium — add D(arr 4, burst 8, arrives mid-run). Show where it lands (Q0!) and who it preempts (the unfair-youth rule: newcomers start high — gaming preview).
3. Hard — implement anti-gaming: track per-job total CPU, demote newcomers with history (or add lottery tickets). Argue what you broke to fix gaming (every scheduler trades — name yours).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| PCB | rows the queues hold (state + queue level live here in real kernels) | [pcb](../../../../glossary/terms.md#pcb) |
| syscall | `nice`/`sched_setscheduler` move jobs between levels from userspace | [syscall](../../../../glossary/terms.md#syscall) |
| heap | runqueues are heap lists/arrays of PCB pointers | [heap](../../../../glossary/terms.md#heap) |

## Further Reading

- OSTEP Ch.8 — rules, starvation, boost, gaming (the chapter this lesson executable-izes).
- `man 7 sched` + `man 1 chrt` — real knobs matching each simulated rule.
- xv6 `kernel/proc.c:scheduler` re-read — single RR queue; imagine 3 of them + demotion (that's next: your capstone scheduler).

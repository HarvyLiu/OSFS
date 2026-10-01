# CFS Intuition — Fairness Is Virtual Time, Not Turns

> Don't count turns. Count weighted time. Run whoever is most behind.

**Type:** Learn
**Languages:** C
**Prerequisites:** 02-mlfq
**College ref:** OSTEP Ch.9 (proportional share: stride), MIT 6.1810 scheduling notes, Linux `kernel/sched/fair.c` (vruntime + weights)
**Time:** ~60 minutes

## Learning Objectives
- Trace min-vruntime picks on 3 tasks (nice 0/5/−5) for 60 ticks and read the shares off
- Implement the sketch: weights table, `vr += 1024/weight` per tick, lowest-first pick
- Explain why vruntime equalizes while CPU shares stay proportional to weight
- Connect nice values to real `ps ni/pri` and to MLFQ's learned levels (different route, same goal)

## Concept in 60s

![vruntime chart](../figures/vruntime.svg)

<!-- source: ../figures/vruntime.excalidraw — open in excalidraw.com to redraw -->

Imagine three tasks arguing over who is owed the CPU. You cannot settle it by counting turns — a turn means different things at different weights. So give each task its own virtual clock. Running one tick advances *its* clock by `1024/weight` — heavyweights (high weight) tick slowly, lightweights fast. You always run the smallest clock, the one most behind in *weighted* time. After 60 ticks with weights 1024/335/3121, the clocks read ~14.0/15.3/13.5 (nearly tied!) while real ticks split 14/5/41 (wildly proportional). Fairness lives in virtual time; throughput lives in real time. `nice` sets weight (table below — each ±1 ≈ ±10%, the only knob users get).

## Simulate It (host — the sketch, no QEMU)

Your sketch lives in `code/cfs.c`. You run three tasks for 60 ticks, always picking the most-behind clock.

```c
#include <stdio.h>

typedef struct { int id; int nice; int weight; double vr; int ticks; } ctask_t;

static int pick_min(ctask_t *ts, int n) {
    int m = 0;
    for (int i = 1; i < n; i++)
        if (ts[i].vr < ts[m].vr || (ts[i].vr == ts[m].vr && ts[i].id < ts[m].id))
            m = i;
    return m;
}

int main(void) {
    ctask_t ts[3] = {{1, 0, 1024, 0.0, 0}, {2, 5, 335, 0.0, 0}, {3, -5, 3121, 0.0, 0}};
    const int T = 60;
    for (int t = 0; t < T; t++) {
        int m = pick_min(ts, 3);
        ts[m].ticks++;
        ts[m].vr += 1024.0 / ts[m].weight;
    }
    for (int i = 0; i < 3; i++)
        printf("task %d nice=%d weight=%d ticks=%d vr=%.2f\n",
               ts[i].id, ts[i].nice, ts[i].weight, ts[i].ticks, ts[i].vr);
    return 0;
}
```

What this does: advances virtual clocks tick-by-tick, always serving the most-behind task — CFS's heartbeat in 25 lines.

| Lines | Code | Why it exists |
|---|---|---|
| 3 | `ctask_t` | id + nice + weight + virtual clock + real count: the two times side by side (the lesson's whole point) |
| 5–11 | `pick_min` | lowest `vr` wins; id tiebreak = deterministic (no test flakiness at t=0 when all read 0.00) |
| 14 | weights 1024/335/3121 | Linux's real table rows for nice 0/5/−5 (`sched_prio_to_weight` — ±1 nice ≈ ×1.25 weight the other way) |
| 18–21 | charge | one real tick → `1024/weight` virtual: nice−5 accrues 0.33/tick (slow clock, picked often), nice+5 accrues 3.06 (fast clock, picked rarely) |
| 22–24 | report | both times printed: `vr` nearly tied, `ticks` wildly split — read them as a pair or miss the idea |

Change X → Y: change all three nice values to `0` (weights all 1024). Verify: `make run` prints `ticks=20` each, `vr=20.00` tied (proves equal weight = plain round-robin — CFS *contains* RR as a special case).

Real output on this workload (copy-paste calibration — your run must match):

```text
task 1 nice=0 weight=1024 ticks=14 vr=14.00
task 2 nice=5 weight=335 ticks=5 vr=15.28
task 3 nice=-5 weight=3121 ticks=41 vr=13.45
```

What this does: the numbers every claim below cites — 60 ticks conserved (14+5+41), shares 23%/8%/68% vs weight shares 22.9%/7.5%/69.6%, clocks within ~1.8 of each other.

| Lines | Code | Why it exists |
|---|---|---|
| `ticks=14/5/41` | real split | proportional to weight (each ±1 from exact ratio — discretization, not error) |
| `vr 13.45–15.28` | virtual tie | spread bounded by one big step (3.06): lag is *capped*, never compounding (the fairness guarantee) |

Change X → Y: run `T = 600` (edit the const). Verify: shares converge closer to 22.9/7.5/69.6 and vr spread stays ~capped (proves bounded lag is structural — it holds at any horizon, unlike MLFQ boost cycles).

## Build It

```bash
make run
make test
```

What this does: prints the calibration lines, then asserts ticks (14/5/41), conservation (sum=60), order (C most, B least), and vr spread (<4.0).

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | diff your three lines against the block above — byte-identical expected |
| `make test` | machine proof | greps the lines + runs `test_main` (double-compare via epsilon box, no `-lm` needed) |

Change X → Y: change B's weight 335→1024 in `cfs.c` only. Verify: test fails on B's ticks (proves the table, not vibes, drives shares — weights are load-bearing data).

## Use It (Linux)

Your kernel does weighted fair queuing *right now*:

```bash
ps -eo pid,ni,pri,comm --sort=-ni | head -8
cat /proc/sys/kernel/sched_min_granularity_ns /proc/sys/kernel/sched_latency_ns 2>/dev/null
```

What this does: lists processes by niceness (user weight hints) plus the two knobs bounding slice size and scheduling period — our `1 tick` and quanta, production-sized.

| Lines | Code | Why it exists |
|---|---|---|
| `ps ... ni` | weight hints live | `NI 19` = weight 15 (starved politely), `NI -20` = weight 88761 (needs root — priority is privilege, as 03/01 showed) |
| `sched_latency_ns` | period target | CFS stretches periods under load so slices stay sane (our fixed T=60 is this knob at toy scale) |

Change X → Y: `nice -n 10 ./build/cfs-demo-loop &` vs default (see Exercises). Verify in `top`: the niced copy gets ~1/4 the CPU of equal competitors (weight ratio in the wild).

## Ship It

Artifact: `outputs/cfs-card.md` — weight table excerpt (−5/0/5), charge formula, pick rule, bounded-lag statement, `ps`/`chrt` verbs. Cite it whenever anyone says "just use priorities" (priorities *are* weights with a clock). You now own fair time — threads will fight over it next.

## Exercises

1. Easy — set T=600, record shares to 1 decimal, confirm 22.9/7.5/69.6 approach (law of large ticks).
2. Medium — add task D nice 10 (weight 110). Predict its ~2.4% share before running, then verify (calibrate your intuition against the table, not hope).
3. Hard — add sleepers: task blocks 10 ticks every 30 (wakes with min-vr — CFS's wakeup preemption in one `if`). Show interactive tasks win latency while hogs keep throughput (the desktop-feels-fast theorem, numeric).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| syscall | `nice`/`sched_setscheduler` adjust weight/placement from userspace | [syscall](../../../../glossary/terms.md#syscall) |
| heap | CFS runqueue is an rbtree of tasks keyed by vr (our array scan is its O(n) toy) | [heap](../../../../glossary/terms.md#heap) |
| PCB | vr + weight live per-task (new card fields beyond 02/03's) | [pcb](../../../../glossary/terms.md#pcb) |

## Further Reading

- OSTEP Ch.9 — stride scheduling (vruntime's integer ancestor — read first, CFS clicks faster).
- Linux `kernel/sched/fair.c` comments (place_entity, vruntime math) — the production 6 lines behind our charge line.
- `man 7 sched` SCHED_OTHER + `man 1 nice` — the user-visible knobs.

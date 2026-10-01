# FIFO, SJF, RR — Three Policies, One Workload

> The workload is fixed. The waiting is a choice. Policy picks who waits.

**Type:** Build
**Languages:** C
**Prerequisites:** 03-context-switch
**College ref:** OSTEP Ch.7–9 (scheduling intro + MLFQ preview), MIT 6.1810 scheduling notes, xv6 file kernel/proc.c (scheduler loop preview)
**Time:** ~75 minutes

## Learning Objectives
- Trace one fixed workload (3 jobs, arrivals 0/1/2, bursts 8/4/2) under three policies by hand
- Implement FIFO, nonpreemptive SJF, and RR(q=2) simulators that print order + completions + averages
- Explain turnaround vs waiting vs response and why SJF minimizes average waiting (given the future)
- Connect policy (who runs) to mechanism (context switch from 02/03 does the running)

## Concept in 60s

![sched timeline](../figures/sched-timeline.svg)

<!-- source: ../figures/sched-timeline.excalidraw — open in excalidraw.com to redraw -->

You run the same three jobs every time: P1(arr 0, burst 8), P2(arr 1, burst 4), P3(arr 2, burst 2). **FIFO** runs arrival order: P1 0–8, P2 8–12, P3 12–14. **SJF** (nonpreemptive, arrival-aware) runs P1 (your only choice at 0), then shortest-available: P3 8–10, P2 10–14. **RR(q=2)** slices: P1 P2 P3 P1 P2 P1 P1, done at 14/10/6. You measure three things: turnaround = completion − arrival (what the user feels), waiting = turnaround − burst (queue pain), response = first-run − arrival (interactivity). SJF wins the averages here *because it cheats*: it knows bursts upfront. Real schedulers must guess (MLFQ next lesson).

## Simulate It (host, no QEMU)

Full program: `code/sched.c`. Same data, three policies, printed Gantts.

```c
#include <stdio.h>

typedef struct { int id, arr, burst, comp; } job_t;

static void report(const char *name, job_t *js, int n) {
    double tsum = 0, wsum = 0;
    printf("%s order:", name);
    for (int i = 0; i < n; i++) printf(" P%d", js[i].id);
    printf(" comp:");
    for (int i = 0; i < n; i++) printf(" P%d=%d", js[i].id, js[i].comp);
    for (int i = 0; i < n; i++) {
        int turn = js[i].comp - js[i].arr;
        tsum += turn;
        wsum += turn - js[i].burst;
    }
    printf(" avg_turn=%.2f avg_wait=%.2f\n", tsum / n, wsum / n);
}

int main(void) {
    job_t fifo[3] = {{1,0,8,8},{2,1,4,12},{3,2,2,14}};
    job_t sjf[3]  = {{1,0,8,8},{3,2,2,10},{2,1,4,14}};
    job_t rr[3]   = {{1,0,8,14},{2,1,4,10},{3,2,2,6}};
    report("FIFO", fifo, 3);
    report("SJF", sjf, 3);
    report("RRq2", rr, 3);
    return 0;
}
```

What this does: hard-codes the hand-traced completions (you verify them below) and reports order + per-job completions + averages — policy comparison in 30 lines.

| Lines | Code | Why it exists |
|---|---|---|
| 3 | `job_t` | id + arrival + burst + completion: the four numbers every policy reasons about |
| 5–16 | `report` | order (who ran), comp (when each finished), averages: turnaround = comp−arr, waiting = turnaround−burst |
| 19 | `fifo` | arrival order P1,P2,P3 with completions 8,12,14 (trace it: P1 0–8 blocks all) |
| 20 | `sjf` | at t=8 both P2,P3 wait → shorter P3 first (8–10), then P2 (10–14) |
| 21 | `rr` | quantum-2 interleaving → completions P1=14,P2=10,P3=6 (computed in Concept) |

Change X → Y: change RR quantum story — set P3 burst 2→4 (edit both `burst` and `comp` by hand-tracing first!). Verify: `make run` averages shift; if your hand-trace was wrong the test below fails (tracing skill > coding skill here).

Derivation you must do once (Gantt — the proof behind the constants):

```text
FIFO: |P1 0-8|P2 8-12|P3 12-14|  comp P1=8 P2=12 P3=14
SJF:  |P1 0-8|P3 8-10|P2 10-14|  comp P1=8 P3=10 P2=14
RRq2: |P1 0-2|P2 2-4|P3 4-6|P1 6-8|P2 8-10|P1 10-12|P1 12-14|
```

What this does: the paper trace every simulator constant comes from — no number in `sched.c` is trusted until its interval appears here.

| Lines | Code | Why it exists |
|---|---|---|
| FIFO row | no preemption, arrival order | convoy effect visible: tiny P3 waits 10 despite burst 2 |
| SJF row | shortest-available-first | P3 jumps P2 at t=8; starvation risk named (long P1-equivalents wait in longer mixes) |
| RR row | 2-unit slices | 7 slices; response beats FIFO (P3 first runs at 4, not 12) at the price of 7 switches (02/03 cost!) |

Change X → Y: re-trace with quantum 4. Verify: fewer slices, P3 completes later than 6 — quantum trades response for switch overhead (the knob CFS auto-tunes later).

## Build It (make the numbers executable + checked)

```bash
make run
make test
```

What this does: prints the three policy lines, then asserts every constant (order, completions, averages to 2 decimals) in `tests/test_main.c`.

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | expect `FIFO ... avg_turn=10.33 avg_wait=5.67`, `SJF ... 9.67/5.00`, `RRq2 ... 9.00/4.33` |
| `make test` | machine proof | greps the three lines + runs C asserts on completions and averages |

Change X → Y: break one constant (e.g. SJF P2 comp 14→13). Verify: test fails naming the policy (proves constants are checked, not decorative).

## Use It (Linux)

Your machine runs CFS, not these toys — compare philosophies:

```bash
chrt --help 2>&1 | head -5
ps -eo pid,ni,stat,cmd | head -6
cat /proc/loadavg
```

What this does: shows real knobs (scheduling policies, niceness, run-queue load) that replace our fixed Gantts.

| Lines | Code | Why it exists |
|---|---|---|
| `chrt --help` | policy list | `SCHED_FIFO/RR/BATCH/IDLE/DEADLINE` — real FIFO/RR exist with priorities + preemption (ours had neither) |
| `ps ... ni` | niceness | `-20..19`: user hint to CFS weight (the civilized descendant of "shortest first") |
| `/proc/loadavg` | queue pressure | `1.2 0.8 ...` = runnable averages; high load = our 3-job toy at datacenter scale |

Change X → Y: run `nice -n 10 sleep 0.1 & nice -n -10 sleep 0.1 2>/dev/null; echo tried` (second may fail without root — that's the lesson). Verify: failure message = priority is privilege (schedulers enforce policy, not just compute it).

## Ship It

Artifact: `outputs/policy-card.md` — FIFO/SJF/RR one-liners (when each wins, pathology: convoy/starvation/switch-storm) + the 3-job numbers as calibration. Cite it in every later "why MLFQ/CFS?" argument.

## Exercises

1. Easy — compute response times (first-run − arrival) for all three policies from the Gantts. Which policy minimizes max response?
2. Medium — add P4(arr 3, burst 6), re-trace all three, update code + tests. Show which policy's average moves most.
3. Hard — implement RR with quantum as argv (`./sched 1` vs `./sched 4`), print switch counts. Argue the response-vs-overhead trade with numbers (this *is* the CFS motivation).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| PCB | the card the scheduler deals (state RUNNABLE→RUNNING) | [pcb](../../../../glossary/terms.md#pcb) |
| syscall | `sched_yield`/sleep block voluntarily; timer preempts involuntarily | [syscall](../../../../glossary/terms.md#syscall) |
| heap | where real runqueues live (arrays/lists of PCBs) | [heap](../../../../glossary/terms.md#heap) |

## Further Reading

- OSTEP Ch.7–9 — the three policies + MLFQ setup (read before next lesson).
- `man 7 sched` — real policies, priorities, `sched_yield` contract.
- xv6 `kernel/proc.c:scheduler` (skim) — round-robin loop you'll recognize instantly.

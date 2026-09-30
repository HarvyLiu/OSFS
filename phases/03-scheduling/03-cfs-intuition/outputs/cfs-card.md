# CFS card

- Rule: run min vruntime. Charge per tick: vr += 1024/weight.
- Weights (Linux): nice -5:3121, 0:1024, 5:335 (±1 ~= x1.25).
- Invariants: ticks conserved; shares proportional; vr spread bounded (~one big step).
- Knobs: sched_latency_ns (period), min_granularity_ns (min slice). See with ps ni/pri.
- Calibrate: 60 ticks, 1024/335/3121 => 14/5/41, vr 14.00/15.28/13.45.

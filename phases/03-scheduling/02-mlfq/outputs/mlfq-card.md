# MLFQ card

- Rules: new->Q0; exhaust->demote; higher non-empty preempts; FIFO within; boost all->Q0 every N.
- Quanta: small top (response, 2-10ms real), bigger mid, FCFS bottom (throughput).
- Starvation: hog sinks, shorts stream above forever. Symptom: Q2 age grows unbounded.
- Boost ~50-100ms in real systems. Price: history loss (gaming: yield-before-exhaust to stay high).
- Calibrate: A3/B10/C3 => 7/16/12, boost8 => 7/16/11.

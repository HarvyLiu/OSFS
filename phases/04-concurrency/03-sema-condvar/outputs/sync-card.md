# Sync card

- Sem: wait (block at 0) / post (add+wake one). Seeds ARE protocol (empty=N, full=0).
- Cond: wait(mutex,cond) sleeps-for-a-reason; ALWAYS while-recheck (spurious/stolen).
- Recipe: pace counts with sems, guard positions with mutex, verify with order-free sums.
- Deadlock: 4 Coffman conditions; kill circular-wait via global order (A-then-B everywhere).
- Costs: spin burns (short), sleep surrenders (long). time + ps-stat tell which you pay.

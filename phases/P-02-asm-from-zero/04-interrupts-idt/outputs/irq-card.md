# IRQ card

- Path: raise -> enabled? run : pend -> handler saves, acks, wakes -> restore -> iret.
- Order: cli FIRST, then save/call/restore, iret last (never sti+ret separately).
- Rules: handlers short (defer work), clear-pending-before-call (no recurse), spurious-safe (guard vectors).
- Userspace: NEVER cli/sti/iret (ring 3 faults). GDB: break handler, info registers eflags, x/3xg $rsp.

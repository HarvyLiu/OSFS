# Runbook: is it the switch?

- Wrong task resumes / ping-pong breaks: `stash` and `load` operands swapped. Check `(%rdi)` vs `%rsi` order.
- Callee-saved garbage (rbx/rbp/r12-r15 wrong after switch): missing/incorrect push/pop pair. Counts must match, order reversed.
- First-switch crash into nowhere: new stack lacks seeded return address. Plant entry point before first `ret`.
- GDB: `break ctx_shape_note`, `info registers rsp`, `x/8xg $rsp` before/after. rsp must equal the PCB field.

# Rings card

- THE rule: allow iff cpl <= dpl (numeric). Up safe, down faults (#GP).
- Shapes: gates (callable doors: dpl+target) vs segments (readable regions: dpl).
- Doors: syscall DPL3 (yours), kdebug DPL0 (never yours), rings 1/2 (empty, enforced).
- Fast: syscall/sysret via MSRs. Slow teaching path: int gates via IDT.
- Inside walls: SMEP (no user-exec in kernel) SMAP (no user-data touch) + seccomp (fewer gates).

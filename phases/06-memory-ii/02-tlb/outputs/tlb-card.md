# TLB card

- Lookup: hit (101: probe+data) / miss (201: +walk). EAT = h*101+(1-h)*201.
- Policy: FIFO/LRU evict; cold misses prime; cyclic > cache thrashes (0 hits!).
- Coherence: unmap => shootdown (other CPUs' entries die); switch => flush or ASID.
- Reach: entries x size. Pressure fix: hugepages, locality, fewer spaces.
- Verbs: perf dTLB-loads/misses; EAT math by hand first.

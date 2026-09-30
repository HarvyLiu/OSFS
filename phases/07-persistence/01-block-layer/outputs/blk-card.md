# Block card

- Verbs: read (cache-first), write (dirty), flush (persist dirties), crash (drop dirties).
- Rules: coherence != durability; flush count = truth events; remove-before-run = determinism.
- Torn: crash mid-flush = half-new (checksum detect, journal repair next).
- Physics: HDD seeks ~ms (random dies); SSD erase-blocks + wear (writes cost elsewhere).
- Read: lsblk ROTA, df budgets, strace -c flush batching.

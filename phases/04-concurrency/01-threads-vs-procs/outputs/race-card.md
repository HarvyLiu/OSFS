# Race card

- Shared: globals, heap, files, code. Private: stack, registers, errno, thread-id.
- Smell: flaky counts/totals, exact-at-small-N, short-at-scale => shared write, missing fence.
- Prove: shrink window (exact?) then grow (short?) — scale is the detector.
- Build: -pthread compile AND link. Join retrieves returns (check them).
- Fix (today): one mutex around the triple. Rule: ONE datum, ONE lock, ALL accessors.

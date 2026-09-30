# Runbook: pointer triage (keep next to GDB)

1. **NULL?** `p == 0` → you never assigned or malloc failed. Check the `if (!p)` path.
2. **Freed / wild?** Address looks huge/tiny or repeats across runs → use-after-free or uninitialized. Re-run under `valgrind` if on host.
3. **Off-by-stride?** You did `p+1` expecting +1 byte but got +4/+8 → remember stride is `sizeof(*p)`. Cast to `char*` to move by bytes.

Verify each fix with `make run` + `make test` before touching QEMU code.

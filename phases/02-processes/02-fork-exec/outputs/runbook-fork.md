# Runbook: fork triage

- `ps ... stat=Z` → zombie: parent forgot `waitpid`. Add wait or adopt via init.
- `fork: Resource temporarily unavailable` → `ulimit -u` / cgroup pids cap. Check `dmesg`.
- Child prints twice / flushes twice → used `exit()` not `_exit()` after fork with buffered stdio.
- `exec: No such file` → PATH or missing binary in minimal rootfs (matters for myos Phase 10).

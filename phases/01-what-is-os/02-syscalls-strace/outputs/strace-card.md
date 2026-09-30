# strace card

- Verbs: -f (follow forks), -e trace=SET (filter), -c (counts), -p PID (attach), -o file.
- Sets: %process %file %network %memory; or names: write,read,openat,close,fork,execve.
- Read: name(args) = result. -1 + E... = errno (see man 2 ERRORS).
- Rules: short reads/writes legal (loop); EINTR retry; man 2 RETURN VALUE first.
- Cost: ~100ns-us per trap; batch (Exercise 2 proves ~100x).

# VM card

- Order: code/data low, heap up, mmap gaps, stack down, kernel top.
- Atom: 4 KiB pages (sysconf queried); touch faults in, munmap releases.
- Ask: mmap ANON (zeroed/private), SHARED (IPC), FILE (libs). Protect: mprotect.
- Read: pmap -x (sized), /proc/PID/maps (truth), layout selfie (inside view).
- Rule: virtual numbers per-proc; same digits, different frames across procs.

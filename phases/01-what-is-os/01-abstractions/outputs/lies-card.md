# Lies card (course map in a pocket)

- Files (07-08): blocks -> named streams+offsets. fds 0/1/2 std; user fds from 3.
- Procs (02-04): cpu -> private machine (pid, fds, space). fork clones, exec replaces, wait reaps.
- Spaces (05-06): ram -> private 0..max. Same numbers, different bytes per proc.
- Counter: syscalls (open/read/write/fork/mmap). Observe: /proc/PID/{fd,maps,status}.

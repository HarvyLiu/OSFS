# QEMU cheatsheet

- Boot: `timeout 5 qemu-system-x86_64 -nographic -kernel build/kernel.elf` (124 = expected timeout kill).
- Quit `-nographic` guest: `Ctrl-A X`.
- Debug: T1 `qemu ... -S -s`, T2 `gdb -ex 'target remote :1234' -ex 'break kernel_main' -ex continue build/kernel.elf`.
- Inspect: `info registers`, `x/8i $pc`, `bt`, `p/x $esp`.

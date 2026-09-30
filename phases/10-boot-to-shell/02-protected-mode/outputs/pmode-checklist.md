# PMode checklist (10/03 builds on this floor)

- Drive byte saved from %dl before any int.
- Kernel linked at 0x8000 (ENTRY kentry first); loader reads from sector 2.
- LOAD_SECTORS measured (stat), not guessed.
- A20 via int15/2401 (past-1MiB unlocked).
- GDT decoded: null, code 9A CF, data 92 CF (4G flat).
- CR0.PE set, FAR jump $0x08 (hidden cache flushed).
- Segments 0x10, stack 0x7000 (below kernel, grows down).
- Freestanding C: no libc, VGA cells + serial, returns to hang.
- qemu -nographic shows message; 124 = success.

# Walk card

- Split (32b): dir=va>>22, tab=(va>>12)&0x3FF, off=va&0xFFF.
- Walk: dir miss -2 -> page miss -3 -> perm -4 -> phys=frame*4096+off.
- Flags: P present, RW writable, US user (kernel-only without).
- Economy: NULL slot = empty 4M for 4B; tables on demand (create on map, never on walk).
- Root: per-PCB pointer = CR3. Switch root = switch universe.

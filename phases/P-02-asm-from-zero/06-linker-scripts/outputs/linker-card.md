# Linker card

- Verbs: ENTRY(sym), SECTIONS, `.` (counter), ALIGN(n), PROVIDE(sym=.), *(name) gather.
- Letters (nm): T text, R rodata, D data, B bss, U undefined-elsewhere, W weak.
- Forensics: objdump -h (sections), nm (symbols), readelf -l (LOAD segments).
- Kernel starter: ENTRY + . = 1M + multiboot-first + rodata/data/bss. Touch order with care.

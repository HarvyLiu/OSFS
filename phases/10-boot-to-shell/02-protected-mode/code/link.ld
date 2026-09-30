/* link.ld -- kernel flat layout: entry stub first at 0x8000, rest follows. */
ENTRY(kentry)
SECTIONS {
    . = 0x8000;
    .text : { *(.text) }
    .rodata : { *(.rodata*) }
    .data : { *(.data) }
    .bss : { *(.bss) *(COMMON) }
}

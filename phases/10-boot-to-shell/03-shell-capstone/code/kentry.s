# kentry.s -- 32-bit entry stub: segments, stack, call kmain, hang.
# Lesson docs/en.md. Linked first at 0x8000 (link.ld ENTRY).
.code32
.globl kentry
kentry:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    movl $0x7000, %esp
    call kmain
hang:
    cli
    hlt
    jmp hang

# boot.s -- 16-bit loader: fetch kernel via BIOS disk, A20, GDT, CR0.PE, far jump.
# Lesson docs/en.md. Linux-only build (16-bit real mode + BIOS services).
.code16
.globl _start
_start:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x7C00, %sp
    movb %dl, drive       # BIOS boot drive number: save NOW (clobbered below)
    sti
    movw $0x800, %ax
    movw %ax, %es         # ES:BX = 0x800:0x0000 = physical 0x8000 (kernel home)
    xorw %bx, %bx
    movb drive, %dl       # restore boot drive for int $0x13
    movb $0x02, %ah       # read sectors
    movb $LOAD_SECTORS, %al  # count from Makefile --defsym (kernel size aware)
    movb $0x00, %ch       # cylinder 0
    movb $0x02, %cl       # sector 2 (sector 1 was us)
    movb $0x00, %dh       # head 0
    int $0x13
    jc disk_fail          # carry = BIOS says no (bad image? wrong drive?)
    movw $0x2401, %ax
    int $0x15             # A20 gate: unlock addresses past 1 MiB
    cli
    lgdt gdt_desc         # teach CPU our two segments (null, code, data)
    movl %cr0, %eax
    orl $0x1, %eax        # PE bit: protection enable (the one-bit revolution)
    movl %eax, %cr0
    ljmp $0x08, $0x8000   # far jump: selector 8 (code) flushes 16-bit decoding
disk_fail:
    movw $failmsg, %si
fail_loop:
    lodsb
    testb %al, %al
    jz fail_hang
    movb $0x0E, %ah
    int $0x10
    jmp fail_loop
fail_hang:
    cli
    hlt
    jmp fail_hang
drive:
    .byte 0
failmsg:
    .asciz "disk read failed"
gdt_start:
    .quad 0x0000000000000000                 # null: required, never used
    .quad 0x00CF9A000000FFFF                 # code: base 0, 4G, exec/read
    .quad 0x00CF92000000FFFF                 # data: base 0, 4G, read/write
gdt_end:
gdt_desc:
    .word gdt_end - gdt_start - 1
    .long gdt_start
.fill 510 - (. - _start), 1, 0
.word 0xAA55

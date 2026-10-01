# boot.s -- 512-byte boot sector: VGA + serial "OSFS boot!", then hang.
# Lesson docs/en.md. Linux-only build (16-bit real mode).
.code16
.globl _start
_start:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %ss
    movw $0x7C00, %sp
    sti
    call serial_init
    movw $0xB800, %ax
    movw %ax, %es         # ES = VGA text cells (direct: no firmware in the path)
    xorw %di, %di         # cell cursor in bytes (+2 per char)
    movb $0x07, %ah       # white-on-black (set once: serial_putc preserves %ax)
    movw $msg, %si
putc:
    lodsb
    testb %al, %al
    jz hang
    movb %al, %es:(%di)   # char cell
    movb %ah, %es:1(%di)  # attribute cell
    addw $2, %di
    call serial_putc      # %al still the char (stores don't clobber)
    jmp putc
hang:
    cli
    hlt
    jmp hang

serial_init:
    movw $0x3F9, %dx
    movb $0x00, %al
    outb %al, %dx
    movw $0x3FB, %dx
    movb $0x80, %al
    outb %al, %dx
    movw $0x3F8, %dx
    movb $0x03, %al
    outb %al, %dx
    movw $0x3F9, %dx
    movb $0x00, %al
    outb %al, %dx
    movw $0x3FB, %dx
    movb $0x03, %al
    outb %al, %dx
    movw $0x3FA, %dx
    movb $0xC7, %al
    outb %al, %dx
    movw $0x3FC, %dx
    movb $0x0B, %al
    outb %al, %dx
    ret

serial_putc:
    pushw %dx
    pushw %ax
wait_ser:
    movw $0x3FD, %dx
    inb %dx, %al
    testb $0x20, %al
    jz wait_ser
    popw %ax
    movw $0x3F8, %dx
    outb %al, %dx
    popw %dx
    ret

msg:
    .asciz "OSFS boot!"

.fill 510 - (. - _start), 1, 0
.word 0xAA55

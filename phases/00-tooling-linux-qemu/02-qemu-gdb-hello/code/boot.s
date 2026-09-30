/* boot.s -- AT&T GAS multiboot entry. Lesson docs/en.md */
.set MAGIC, 0x1BADB002
.set FLAGS, 0x0
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .text
.globl _start
_start:
    movl $stack_top, %esp
    call kernel_main
    cli
hang:
    hlt
    jmp hang

.section .bss
.space 16384
stack_top:

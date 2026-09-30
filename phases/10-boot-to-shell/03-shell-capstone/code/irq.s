# irq.s -- timer ISR stub + stack-switch primitive. 32-bit, called from IDT/C.
# Lesson docs/en.md. Linked into kernel (link.ld covers .text).
.code32
.globl timer_stub
timer_stub:              # CPU pushed eflags/cs/eip; handler runs; iret returns.
    pushal
    call timer_handler
    popal
    iret

.globl switch_to
switch_to:               # switch_to(uint32_t *old_sp, uint32_t new_sp)
    movl 4(%esp), %eax   # old_sp pointer
    movl 8(%esp), %ecx   # new_sp value
    pushl %ebx
    pushl %esi
    pushl %edi
    pushl %ebp           # callee-saved, 02-style: switch preserves the contract
    movl %esp, (%eax)    # park current stack
    movl %ecx, %esp      # ride the other stack
    popl %ebp
    popl %edi
    popl %esi
    popl %ebx
    ret                  # pops return address from the NEW stack: first ride
                         # lands on the prepared entry (garbage regs, honest comment)

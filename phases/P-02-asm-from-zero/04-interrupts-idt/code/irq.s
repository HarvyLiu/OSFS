# irq.s -- handler skeleton reference. Assembled, NEVER called on host.
# cli/sti/iret are privileged: executing in userspace faults. Lesson docs/en.md.
.text
.globl irq_shape_note
irq_shape_note:
    ret
# Bare-metal sequence (Phase 10 implements for real):
# cli
# pushq %rax; pushq %rcx; pushq %rdx; pushq %rsi; pushq %rdi
# call irq_dispatch
# popq %rdi; popq %rsi; popq %rdx; popq %rcx; popq %rax
# iretq

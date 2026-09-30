# frame.s -- textbook prologue/epilogue add. Lesson docs/en.md.
# System V (Linux/WSL/Docker for execution). Assembles anywhere GAS runs.
.text
.globl frame_add
frame_add:
    pushq %rbp
    movq %rsp, %rbp
    movl %edi, -4(%rbp)
    movl %esi, -8(%rbp)
    movl -4(%rbp), %eax
    addl -8(%rbp), %eax
    popq %rbp
    ret

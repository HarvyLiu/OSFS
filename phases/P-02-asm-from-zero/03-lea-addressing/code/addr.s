# addr.s -- scaled load, lea math, field load. Lesson docs/en.md.
# System V (Linux/WSL/Docker for execution). Assembles anywhere GAS runs.
.text
.globl scaled_get
scaled_get:
    movl (%rdi,%rsi,4), %eax
    ret

.globl mul3add
mul3add:
    leal (%rdi,%rdi,2), %eax
    ret

.globl field_state
field_state:
    movl 4(%rdi), %eax
    ret

# switch.s -- get_rsp helper + illustrated full-switch shape. Lesson docs/en.md.
# AT&T GAS, x86-64. Assembles on host (cc -c); executes bare-metal in Phase 10.
.text
.globl get_rsp
get_rsp:
    movq %rsp, %rax
    ret

# Full-switch shape (reference; not linked into host sim):
# ctx_switch(old_rsp_ptr in %rdi, new_rsp in %rsi):
#   pushq %rbx; pushq %rbp; pushq %r12; pushq %r13; pushq %r14; pushq %r15
#   movq %rsp, (%rdi)
#   movq %rsi, %rsp
#   popq %r15; popq %r14; popq %r13; popq %r12; popq %rbp; popq %rbx
#   ret
.globl ctx_shape_note
ctx_shape_note:
    ret

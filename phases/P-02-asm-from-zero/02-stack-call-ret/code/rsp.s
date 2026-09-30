# rsp.s -- return live stack pointer. Lesson docs/en.md.
.text
.globl get_rsp
get_rsp:
    movq %rsp, %rax
    ret

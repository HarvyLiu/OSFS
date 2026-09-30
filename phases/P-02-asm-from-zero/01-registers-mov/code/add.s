# add.s -- AT&T GAS add2. Lesson docs/en.md.
# x86-64 System V (Linux/WSL/Docker): arg1 in %edi, arg2 in %esi, return in %eax.
# (Windows x64 uses %ecx/%edx instead -- same mnemonics, different registers.)
.text
.globl add2
add2:
    movl %edi, %eax
    addl %esi, %eax
    ret

# ASM cheatsheet (AT&T first)

- Sigils: `$` number, `%` register, no sigil = memory. `movl $1,%eax` = Intel `mov eax,1`.
- Widths: `b`=8 `w`=16 `l`=32 `q`=64. Match the data (`int`=`l`).
- Order: AT&T `src,dst`; Intel `dst,src`.
- 15 you need: mov, lea, add, sub, imul, and, or, xor, cmp, test, jmp, je/jne, call, ret, push, pop (+ in/out, cli/sti/hlt for OS).
- GDB: `break fn`, `run`, `info registers eax edi esi esp eip`, `si`, `x/4xw $esp`, `layout asm`.

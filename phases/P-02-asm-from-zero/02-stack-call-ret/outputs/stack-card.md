# Stack card

- Prologue: `pushq %rbp` / `movq %rsp,%rbp`. Epilogue: `popq %rbp` / `ret` (`leave` = both moves).
- Locals: `-N(%rbp)`. Spilled args above `+8(%rbp)` past return addr.
- GDB: `bt`, `info registers rbp rsp rip`, `x/4xg $rsp`, `info frame`.
- x86 grows DOWN: deeper = smaller rsp. Red zone: 128B below rsp (leaf-only; handlers beware).

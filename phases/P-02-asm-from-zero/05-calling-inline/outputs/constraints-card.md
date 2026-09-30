# Constraints card

- Shape: `__asm__(T : OUT : IN : CLOBBER)`. Numbers: outputs first (%0..), then inputs.
- Letters: `=r` write-any, `+r` read/write-any, `r` read-any, `a` eax, `b/c/d` siblings, `Nd` imm8-or-dx, `I` imm32.
- Modifiers: `=` write-only, `+` read-write, `&` early-clobber (don't reuse an input reg).
- volatile: hardware pokes + timing = always. memory: RAM touched beyond outputs = confess. cc: flags changed = confess.
- Decoded: outb `"outb %0,%1"::"a"(v),"Nd"(p)`; inb `"inb %1,%0":"=a"(r):"Nd"(p)`.

# Hex card

- Nibbles: 0=0000 1=0001 ... 8=1000 9=1001 A=1010 B=1011 C=1100 D=1101 E=1110 F=1111
- `<<n` = x2^n, `>>n` = /2^n. `& mask` keeps bits, `| mask` sets, `^ mask` flips.
- GDB: `p/x` hex, `p/t` binary, `p/d` decimal. `x/4xb $esp` = 4 bytes hex.
- Magic numbers: 0x1000=4096 (page), 0x100000=1M (kernel load), 0x3F8=1016 (COM1).

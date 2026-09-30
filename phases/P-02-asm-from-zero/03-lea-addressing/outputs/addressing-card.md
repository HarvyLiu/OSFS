# Addressing card

- Formula: `offset(base,index,scale)`, scale in {1,2,4,8}. AT&T parens = deref.
- `mov` visits memory. `lea` computes and stops (no touch).
- Scales: char 1, short 2, int/float 4, long/double/pointer 8.
- Struct field = base+offset. Array elem = base+i*size. Page = base+i*4096+off.
- GDB: `x/xw addr` (word hex), `x/4xw base` (4 words), `p/x $rdi+$rsi*4`.

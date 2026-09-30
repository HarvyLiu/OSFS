# Translate card

- Seg: phys=base[seg]+off; faults: bad seg (-1), off>=limit (-2).
- Page (4K): vpn=va>>12, off=va&0xFFF; frame=table[vpn] (fault !present); phys=frame*4096+off.
- Sizes: 4K (12b) / 2M (21b) / 1G (30b): split moves, shape stays.
- Bits: P resent, RW, U ser (protection+swap ride here).
- Verbs: getconf PAGESIZE; maps-alignment check via &0xFFF.

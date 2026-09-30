# Allocator card (growing)

- Bump: cursor+align+fit (20 lines). No single free; reset all. Fits: boot, frames, scratch.
- Free-list: headers link empties; first/best-fit search; coalesce neighbors. Fits: general heap.
- Buddy: pow2 blocks, xor-buddy split/merge. Fits: page frames, low external waste.
- Rule: match tool to lifetime (arena->bump, mixed->list, pages->buddy).

// bump.rs -- boot allocator starter for Phase 10. Lesson 06/01.
// Swap &[u8] arena for physical range + page rounding at bring-up.
pub fn align_up(p: usize, a: usize) -> usize {
    debug_assert!(a.is_power_of_two());
    (p + a - 1) & !(a - 1)
}

pub struct Bump<'a> {
    arena: &'a mut [u8],
    next: usize,
}

impl<'a> Bump<'a> {
    pub fn new(arena: &'a mut [u8]) -> Self {
        Bump { arena, next: 0 }
    }

    pub fn alloc(&mut self, size: usize, align: usize) -> Option<*mut u8> {
        let base = self.arena.as_mut_ptr() as usize;
        let aligned = align_up(base + self.next, align);
        let off = aligned - base;
        let end = off.checked_add(size)?;
        if end > self.arena.len() {
            return None;
        }
        self.next = end;
        Some(aligned as *mut u8)
    }

    pub fn reset(&mut self) {
        self.next = 0;
    }
}

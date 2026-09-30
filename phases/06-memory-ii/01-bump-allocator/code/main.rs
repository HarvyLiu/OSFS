// main.rs -- bump allocator over a borrowed arena. Lesson docs/en.md.
// Build: rustc --edition 2021 main.rs. Tests: rustc --edition 2021 --test main.rs.
struct Bump<'a> {
    arena: &'a mut [u8],
    next: usize,
}

fn align_up(p: usize, a: usize) -> usize {
    debug_assert!(a.is_power_of_two());
    (p + a - 1) & !(a - 1)
}

impl<'a> Bump<'a> {
    fn new(arena: &'a mut [u8]) -> Self {
        Bump { arena, next: 0 }
    }

    fn alloc(&mut self, size: usize, align: usize) -> Option<*mut u8> {
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

    fn used(&self) -> usize {
        self.next
    }

    fn free_bytes(&self) -> usize {
        self.arena.len() - self.next
    }

    fn reset(&mut self) {
        self.next = 0;
    }
}

fn main() {
    let mut backing = vec![0u8; 64];
    let mut b = Bump::new(&mut backing);
    let a = b.alloc(16, 8).expect("fits");
    let c = b.alloc(8, 16).expect("fits");
    println!("a={a:p} c={c:p} used={} free={}", b.used(), b.free_bytes());
    println!("aligned16={}", (c as usize) % 16 == 0);
    let oom = b.alloc(64, 1);
    println!("oom={oom:?}");
    b.reset();
    println!("after reset used={} free={}", b.used(), b.free_bytes());
}

#[cfg(test)]
mod checks {
    use super::*;

    #[test]
    fn sequence_advances() {
        let mut backing = vec![0u8; 64];
        let mut b = Bump::new(&mut backing);
        let a = b.alloc(16, 8).unwrap() as usize;
        let c = b.alloc(8, 8).unwrap() as usize;
        assert!(c >= a + 16); // second starts after first (+possible gap)
        assert!(b.used() >= 24);
    }

    #[test]
    fn alignment_honored() {
        let mut backing = vec![0u8; 64];
        let mut b = Bump::new(&mut backing);
        let p = b.alloc(1, 16).unwrap() as usize;
        assert_eq!(p % 16, 0);
    }

    #[test]
    fn oom_is_none() {
        let mut backing = vec![0u8; 16];
        let mut b = Bump::new(&mut backing);
        assert!(b.alloc(16, 1).is_some());
        assert!(b.alloc(1, 1).is_none()); // exhausted: honest None
    }

    #[test]
    fn reset_reclaims_all() {
        let mut backing = vec![0u8; 32];
        let mut b = Bump::new(&mut backing);
        assert!(b.alloc(32, 1).is_some());
        assert!(b.alloc(1, 1).is_none());
        b.reset();
        assert_eq!(b.used(), 0);
        assert!(b.alloc(32, 1).is_some()); // full arena again
    }

    #[test]
    fn zero_size_tolerated() {
        let mut backing = vec![0u8; 16];
        let mut b = Bump::new(&mut backing);
        let before = b.used();
        assert!(b.alloc(0, 8).is_some());
        assert_eq!(b.used(), before); // advances nothing
    }
}

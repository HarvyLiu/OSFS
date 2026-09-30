# Bump Allocator — The World's Simplest Honest malloc

> Keep a pointer. Round it up. Hand out the gap. Freeing is forgetting (until reset).

**Type:** Build
**Languages:** Rust
**Prerequisites:** 03-page-tables
**College ref:** OSTEP Ch.14 (free-list vs bump tradeoffs), MIT 6.1810 kalloc notes (page-granular cousin), xv6-rust allocator (this shape, grown up)
**Time:** ~75 minutes

## Learning Objectives
- Trace `next` across allocations with alignment gaps using a heap diagram
- Implement `align_up`, `alloc`, `reset`, `used`/`free_bytes` over a borrowed byte arena
- Explain why bump can't free singly (no headers/metadata) and where that fits (arenas, boot, frames)
- Connect alignment to P-02/06 (`ALIGN`), PTE page granularity, and Rust's `Layout`

## Concept in 60s

![bump pointer](../figures/bump-pointer.svg)

<!-- source: ../figures/bump-pointer.excalidraw — open in excalidraw.com to redraw -->

One cursor `next` starts at arena base. `alloc(size, align)`: round `next` up to `align` (`align_up`: `(p + a-1) & !(a-1)`, power-of-two only), check fit (`aligned + size <= end`), hand out `aligned`, set `next = aligned + size`. No headers, no lists, no reuse: individual `free` is impossible (nothing records sizes) — `reset` reclaims *everything* (arena discipline: request phase, then reset). Perfect for boot (map tables, then never free), frames (page-granular bump in `kalloc`'s youth), and short-lived scratch. General malloc needs headers/buddies (next lessons); bump needs 20 lines.

## Simulate It (host rustc — arena borrowed, no OS needed)

Full program: `code/main.rs`. Borrows a `Vec<u8>` as the arena (host stand-in for a physical region).

```rust
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
            return None; // OOM: honest None, like P-03/02's table-full
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
```

What this does: hands out two aligned chunks (watching the gap), proves OOM honesty, resets to zero — arena lifecycle complete.

| Lines | Code | Why it exists |
|---|---|---|
| 1–4 | `Bump<'a>` borrows | lifetime `'a`: arena outlives allocator (borrow checker enforces use-after-arena at compile time — P-03/01's rules paying rent) |
| 6–9 | `align_up` | bit trick for power-of-two `a` (`debug_assert` documents the precondition; non-pow2 aligns *wrong*, silently — hence the assert) |
| 14–26 | `alloc` | base+next → align → bounds-check with `checked_add` (overflow-safe: `usize::MAX` sizes fail clean, not wrap) → advance → raw pointer out |
| 17 | `checked_add` + `?` | `?` on `Option` returns `None` early (the `Result`-less cousin of P-03/02's `Err` paths — same honesty, lighter type) |
| 28–38 | used/free/reset | accounting + whole-arena reclaim (the *only* free: discipline over data structures) |
| 44–49 | demo flow | 16@8 then 8@16 (second may gap for alignment — read `used` to count the tax), OOM `None`, reset to 0 |

Change X → Y: change second alloc align `16` → `1`. Verify: `used` shrinks by the former gap (alignment tax itemized — the padding P-02/06 charged silently, now metered per call).

## Build It

```bash
make run
make test
```

What this does: runs the lifecycle, then runs 5 inline tests (sequence, alignment, OOM, reset-reuse, zero-size edge).

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | expect `aligned16=true`, `oom=None`, `after reset used=0 free=64` |
| `make test` | machine proof | same struct, asserted (arena math is where allocators die — pin every number) |

Change X → Y: request `size=0` (edge!). Verify: returns `Some` without advancing (zero-sized types exist in Rust — the allocator must tolerate, not trap; test pins it).

## Use It (Linux)

Real arenas behave identically at all scales:

```bash
./build/bump-rs
rustc --edition 2021 --test code/main.rs -o /tmp/bump-test -D warnings && /tmp/bump-test 2>&1 | tail -3
```

What this does: runs verbs + suite raw (the two commands `make` wraps — habit: unwrap once per lesson).

| Lines | Code | Why it exists |
|---|---|---|
| suite tail | proof | 5 passed (sequence/alignment/OOM/reset/zero-size — the allocator's contract surface) |

Change X → Y: `RUSTFLAGS="-Z print-type-sizes" ...`? Nightly-only — skip; instead `size_of::<Bump>()` print added scratch-side (fat pointer + usize = 24B — the allocator's own footprint, smaller than one PTE page by 170×).

## Ship It

Artifact: `outputs/bump.rs` — `align_up` + `Bump` as Phase 10's boot allocator starter (swap `&mut [u8]` for physical range + page-rounding then). Plus `outputs/allocator-card.md` (bump vs free-list vs buddy tradeoffs, one line each).

## Exercises

1. Easy — allocate 1,2,4,8,... sizes @8, record `used` after each (staircase shows alignment tax per size class).
2. Medium — add `alloc_page()` (4096-aligned 4096B, for 05/03's tables!) + test (page tables allocate pages — the two lessons shake hands here).
3. Hard — implement `Buddy` order-0..4 over the same arena (split/merge with buddy-xor `buddy = addr ^ (1<<order)`): compare external waste vs bump on random churn (the fragmentation exhibit from 05/02, now with your two allocators as specimens).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| heap | this lesson *is* a heap (owned region + cursor + discipline) | ../../glossary/terms.md#heap |
| address | handed-out pointers are arena offsets made absolute (base+cursor) | ../../glossary/terms.md#address |
| page | page-granular bump (Exercise 2) feeds multi-level tables directly | ../../glossary/terms.md#page |

## Further Reading

- OSTEP Ch.14 — allocator zoo (bump's corner: fastest, least general — know when).
- Rust `core::alloc::Layout` docs — size+align bundled (the std-facing shape of our two args).
- xv6-rust allocator (skim) — free-list grown-up: headers where bump has none.

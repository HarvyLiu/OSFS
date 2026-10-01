# no_std, panic_handler, unsafe — Rust Without a Floor

> std leaves. core stays. You bring the panic room and the sharp knives stay sheathed.

**Type:** Learn
**Languages:** Rust
**Prerequisites:** 03-traits-generics
**College ref:** OSTEP Ch.13 (kernels can't assume libc), MIT 6.1810 rust kernel modules (no_std reality), xv6-rust `main.rs` (`#![no_std]` + panic handler)
**Time:** ~90 minutes

## Learning Objectives
- Trace what `std` provides vs `core` keeps using a split-build (one file each)
- Implement `#[panic_handler]`, `unsafe` raw-pointer read fenced in a safe wrapper, and wrapping arithmetic
- Explain why `println!`/`Vec` vanish in `no_std` (they need allocation/OS) and what replaces them
- Connect `unsafe` boundaries to P-02/05's inline ASM (same rule: fence the sharp parts, expose safe verbs)

## Concept in 60s

![no_std layers](../figures/no-std.svg)

<!-- source: ../figures/no-std.excalidraw — open in excalidraw.com to redraw -->

You boot with no OS beneath you, so `std` leaves and `core` stays: `std` = `core` (always available: types, traits, `Option`/`Result`, atomics) + `alloc` (needs an allocator: `Vec`/`String`) + OS services (threads, files, `println!`, backtraces). `#![no_std]` unplugs the last two: your crate links `core` only. Two debts come due: (1) a `#[panic_handler]` (nowhere to print + unwind — usually log-to-serial + halt, like Tooling 02's `hlt` loop); (2) no `main` convention (`#![no_main]` + your own entry, like `_start`). `unsafe` marks the five sharp powers (raw-pointer deref, mutable statics, inline ASM, `extern`, unsafe-trait impls) — fenced in tiny blocks, wrapped in safe functions whose *contracts* callers can trust without reading the blade.

## Simulate It (host rustc — both floors, side by side)

`code/nostd.rs` (the floorless half — compiles to a static lib, never runs hosted):

```rust
// nostd.rs -- core only. Lesson docs/en.md. No std, no main, own panic room.
#![no_std]
#![no_main]

use core::panic::PanicInfo;

#[panic_handler]
fn panic_room(info: &PanicInfo) -> ! {
    let _ = info;
    loop {
        core::hint::spin_loop();
    }
}

#[no_mangle]
pub extern "C" fn kern_add(a: i64, b: i64) -> i64 {
    a.wrapping_add(b)
}

/// Read one byte through a raw pointer. Caller promises validity.
pub unsafe fn peek(addr: *const u8) -> u8 {
    *addr
}
```

What this does: proves `core`-only compiles — panic room that halts, wrapping add (overflow *defined*), and one honestly-marked sharp edge.

| Lines | Code | Why it exists |
|---|---|---|
| 3–4 | `#![no_std/#![no_main]` | unplug std + hosted entry (inner attributes `#!` = whole crate; without these the compiler demands `main` + links std) |
| 8–14 | `panic_handler -> !` | `!` = never returns (halt loop + `spin_loop` hint — Tooling 02's `hang: hlt; jmp hang`, Rust-spelled) |
| 17–19 | `wrapping_add` | overflow *wraps* by contract (plain `+` panics in debug — kernels pick wrapping deliberately, like C's unsigned) |
| 22–24 | `unsafe fn peek` | raw deref fenced + named: unsafety is *caller's* burden now (safe wrappers come in `main.rs`) |

Change X → Y: delete `loop {}` body (leave empty `-> !`). Verify: `unreachable code`-style error? Actually empty diverging fn errors (`mismatched types` — `!` uninhabited: you must diverge; the type system enforces the halt).

`code/main.rs` (the safe-verbs half — runs hosted, fences the blades):

```rust
// main.rs -- safe wrappers over sharp edges + wrapping math. Lesson docs/en.md.
fn safe_peek_byte(slice: &[u8], i: usize) -> Option<u8> {
    if i < slice.len() {
        // SAFETY: i < len, so the pointer is in-bounds and alive (borrowed).
        Some(unsafe { *slice.as_ptr().add(i) })
    } else {
        None
    }
}

fn main() {
    let data = [10u8, 20, 30];
    println!("peek1={:?} peek9={:?}", safe_peek_byte(&data, 1), safe_peek_byte(&data, 9));
    let top = i64::MAX;
    println!("wrap={} plain-panics-in-debug", top.wrapping_add(1));
    println!("checked={:?}", top.checked_add(1));
}
```

What this does: bounds-checks *then* derefs raw (the fence pattern), shows wrapping vs checked overflow side by side.

| Lines | Code | Why it exists |
|---|---|---|
| 2–8 | safe wrapper | `unsafe` block (3 tokens) inside a safe `fn`: callers get `Option` honesty without `unsafe` at *their* site (the boundary pattern for all kernel code) |
| 3 | `SAFETY:` comment | convention with teeth: states the proof obligation (in-bounds + alive) so reviewers audit the claim, not the pointer |
| 14–15 | wrap vs checked | `wrapping_add` = modular (kernel counters), `checked_add` = `None` on overflow (fallible paths) — overflow is a *choice* in Rust, unlike C's UB lottery |

Change X → Y: change `add(i)` to `add(i+1)` mentally → returns `None` at the edge (proves the guard, not the data, decides — off-by-ones die here, loudly).

## Build It

```bash
make run
make test
```

What this does: runs the safe verbs, compiles the `no_std` lib + checks `panic_room` survived as a symbol, runs 4 inline tests.

| Lines | Code | Why it exists |
|---|---|---|
| `nostd` lib target | `--crate-type=lib` | `no_main` crates link, don't run (like `.o` files in P-01/06 — resolution later, at kernel link) |
| `nm` grep | symbol proof | `panic_room` present = the panic debt paid (missing handler = link error naming it — try deleting to see) |
| `--test` | machine proof | wrapping/checked/peek-some/peek-none on the real functions |

Change X → Y: delete `#[panic_handler]`, rebuild lib. Verify: `error: language item required: panic_handler` (the debt itemized — the compiler invoices missing floors).

## Use It (Linux)

Cores leave traces even hosted:

```bash
rustc --print sysroot
ls $(rustc --print sysroot)/lib/rustlib/*/lib/ | grep -E "^libcore|^libstd" | head -4
rustc --edition 2021 --test code/main.rs -o /tmp/ns-test -D warnings && /tmp/ns-test 2>&1 | tail -3
```

What this does: finds your toolchain's `core` vs `std` rlibs (the two floors as files), then runs the suite raw.

| Lines | Code | Why it exists |
|---|---|---|
| `sysroot` libs | floor files | `libcore-*.rlib` (always) vs `libstd-*.rlib` (hosted only) — `no_std` simply stops linking the second |
| `--test` raw | unwrapped proof | same command `make test` hides (habit from every primer lesson: run raw once) |

Change X → Y: `rustc --print target-list | grep -E "x86_64-unknown-none|thumbv7" | head -3`. Verify: bare-metal targets exist with *no* OS suffix (your kernel's future `--target` — the floor choice becomes a flag).

## Ship It

Artifact: `outputs/nostd-starter.rs` — `#![no_std/#![no_main]`, panic room, `kern_add`, `peek` + SAFETY template comment. `cp` it as every bare-metal Rust file's first 25 lines (Phase 10's allocator starts here).

## Exercises

1. Easy — add `safe_write_byte(&mut [u8], i, v) -> bool` (bounds-check + raw write, `false` past the end). Prove with tests both sides of the edge.
2. Medium — implement `pstrlen(addr: *const u8, max: usize) -> Option<usize>` (C-string scan with a cap, `unsafe` inside, safe face — this *is* the syscall-argument pattern: user pointer in, validated length out).
3. Hard — `cargo init --bin ns &&` add `#![no_std]` + panic handler, watch *what breaks* (`println!`, `Vec`, `std::env` — catalog each error, then fix with `core`/`alloc`-free rewrites: the porting rehearsal for kernel modules).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| panic_handler | `no_std`'s required never-returning panic room (serial + halt) | [panic_handler](../../../../glossary/terms.md#panic_handler) |
| freestanding | C's word for the same floorlessness (`-ffreestanding` ≈ `#![no_std]`) | [freestanding](../../../../glossary/terms.md#freestanding) |
| heap | gone in bare `no_std` (no `Vec`/`String` until *you* provide `alloc` — Memory II) | [heap](../../../../glossary/terms.md#heap) |
| address | raw pointers *are* addresses with lifetimes erased (handle like P-01/03, fenced) | [address](../../../../glossary/terms.md#address) |

## Further Reading

- Rust Book Ch.16-appendix-ish (`no_std` + `unsafe` chapters) + Embedded Book Ch.2–3 (panic handlers, memory-mapped I/O via raw pointers).
- `core::hint::spin_loop` + `core::num` docs (wrapping/checked/saturating menu — pick per call site).
- xv6-rust `main.rs` head (25 lines: attributes + handler + entry — now fully legible).

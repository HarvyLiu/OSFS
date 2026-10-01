# C FFI — Shaking Hands Across the Language Fence

> C layout in, Rust safety out. repr(C) aligns the bytes; extern "C" aligns the calls.

**Type:** Learn
**Languages:** Rust, C
**Prerequisites:** 04-no-std-unsafe
**College ref:** OSTEP Ch.13 (kernels mix languages per subsystem), MIT 6.1810 C/RISC-V calling notes (ABI is the handshake), xv6-rust FFI shims (real `extern "C"` borders)
**Time:** ~75 minutes

## Learning Objectives
- Trace a `#[repr(C)]` struct's layout against C's `offsetof` expectations field by field
- Implement `extern "C"` both directions (Rust exposes, Rust declares) with `#[no_mangle]`
- Explain why kernels set `panic = "abort"` and what `unsafe` at the fence costs/guarantees
- Connect FFI to Phase 10 (Rust allocator called from C traps) and to P-02/05 (same ABI, new alphabet)

## Concept in 60s

![ffi fence](../figures/ffi-fence.svg)

<!-- source: ../figures/ffi-fence.excalidraw — open in excalidraw.com to redraw -->

C and Rust agree on *machine* things: System V argument registers (P-02/01), struct field offsets (P-01/05), symbol names in objects (P-02/06). They disagree on *language* things: name mangling (`_ZN...` vs plain), ownership (P-03/01), panics (unwinding needs a runtime). The fence pieces: `#[repr(C)]` (Rust struct, C layout rules — field order + padding identical), `extern "C"` (C calling convention for this symbol), `#[no_mangle]` (keep the plain name so C's `call kern_add` links), `unsafe` at every crossing (both sides promise validity: non-null, alive, right size). Host demo links nothing cross-language (MSVC-vs-MinGW fences differ!); it *proves each side separately* — the real link happens in the kernel build with one matching target (the rule documented below).

## Simulate It (host rustc + cc — each side proven, fence inspected)

`code/abi.rs` (the shared shape, written twice — once per language, layouts must agree):

```rust
// abi.rs -- repr(C) PCB Half. Lesson docs/en.md.
#[repr(C)]
pub struct PcbFfi {
    pub pid: i32,
    pub state: i32,
    pub rsp: u64,
}

pub const PCB_STATE_RUNNING: i32 = 2;

#[no_mangle]
pub extern "C" fn kern_add(a: i64, b: i64) -> i64 {
    a.wrapping_add(b)
}
```

What this does: publishes a C-compatible struct + constant + function — the exact surface a C kernel would `#include` and `call`.

| Lines | Code | Why it exists |
|---|---|---|
| 2 | `#[repr(C)]` | C layout: field order kept, padding per C rules (default Rust may reorder for packing — fast, unlinkable) |
| 3–7 | `i32/i32/u64` | fixed widths (C `int` varies! LLP64-vs-LP64 from 05/01 — `i32`/`u64` never wobble, so FFI uses them, never `int`-sized guesses) |
| 12–15 | `no_mangle + extern "C"` | plain symbol + C ABI (C's linker sees `kern_add`, not `_ZN...`; args ride `%rdi/%rsi` per P-02/01) |

```c
// caller.c -- the C side of the fence. Compiled (-c) only on host:
// real linking needs ONE matching target (kernel build), not MSVC+MinGW mixed.
// Lesson docs/en.md.
#include <stdint.h>

struct PcbFfi {
    int32_t pid;
    int32_t state;
    uint64_t rsp;
};

#define PCB_STATE_RUNNING 2

extern int64_t kern_add(int64_t a, int64_t b);

int call_it(void) {
    struct PcbFfi p;
    p.pid = 1;
    p.state = PCB_STATE_RUNNING;
    p.rsp = 0;
    return (int)(kern_add(40, 2) == 42) + p.pid;
}
```

What this does: mirrors the struct/constant/function in C — same offsets, same symbol, same ABI (compile-checked; linked in the kernel, not here).

| Lines | Code | Why it exists |
|---|---|---|
| 5–9 | mirror struct | `int32_t/int32_t/uint64_t` = `i32/i32/u64` (widths matched deliberately — the #1 FFI bug is `long`, 4B-vs-8B per platform) |
| 13 | `extern` decl | "defined in Rust": compiles to an unresolved `call` (P-01/06's `U` symbol — promise, delivery later) |
| 15–21 | user | fills the struct, calls across (would print 43 = 42-proof + pid — run it for real in Phase 10's unified build) |

Change X → Y: change C's `uint64_t rsp` to `uint32_t rsp`. Verify: still compiles (`-c` can't see across!) — then read why FFI demands *layout tests* (below): compile-passing lies are the fence's special hazard.

`code/main.rs` (layout truth + safe-face demo, runs hosted):

```rust
// main.rs -- fence verification: sizes, offsets, calls. Lesson docs/en.md.
use std::mem::{size_of, align_of};

#[repr(C)]
struct PcbFfi {
    pid: i32,
    state: i32,
    rsp: u64,
}

fn off_of_pid() -> usize {
    0
}

fn main() {
    println!("size={} align={} pid_off={}", size_of::<PcbFfi>(), align_of::<PcbFfi>(), off_of_pid());
    // Taking addresses needs no unsafe; dereferencing raw pointers does.
    let p = PcbFfi { pid: 1, state: 2, rsp: 0xABCD };
    let base = &p as *const PcbFfi as usize;
    let off_state = &p.state as *const i32 as usize - base;
    let off_rsp = &p.rsp as *const u64 as usize - base;
    // SAFETY: &p.pid is a live, aligned, in-bounds place of this stack value.
    let pid_via_raw = unsafe { *(&p.pid as *const i32) };
    println!("state_off={off_state} rsp_off={off_rsp} rsp_val={:#x} pid_raw={pid_via_raw}", p.rsp);
}
```

What this does: prints size/align/offsets the C side must match byte-for-byte (4/4/8 layout: pid@0, state@4, rsp@8, size 16).

| Lines | Code | Why it exists |
|---|---|---|
| 12–14 | `off_of_pid` = 0 | first field at zero (anchor for the arithmetic below — trivial, load-bearing) |
| 18–20 | raw offset math | differences of live-reference addresses (in-bounds by construction — `unsafe` fenced, P-03/04's pattern, now measuring layout) |
| 21 | values echoed | `rsp` round-trips (`0xabcd` — the bytes C would read at offset 8, previewed here) |

Change X → Y: remove `#[repr(C)]`, rerun. Verify: offsets *may* still print 0/4/8 (Rust *happens* to agree here — the lie that burns: without the attribute, agreement is luck, and luck revokes silently on upgrade).

## Build It

```bash
make run
make test
```

What this does: runs layout truth, compiles the C mirror (`-c`, syntax+offsets human-checked), builds the Rust staticlib shape, runs 4 layout/call tests.

| Lines | Code | Why it exists |
|---|---|---|
| `caller.o` via `-c` | C-side proof | compiles = syntax + types sane (layout *equality* needs the test below — compile can't see across) |
| `--crate-type=staticlib` | kernel artifact shape | `.lib/.a` carrying `kern_add` + unwound-free code (with `panic="abort"` in real kernel builds — no unwinder below) |
| `--test` | machine proof | size/align/offsets + `kern_add` vectors, including the `u32`-vs-`u64` trap from Change X |

Change X → Y: `grep -c no_mangle abi.rs` → 1, then `nm` the staticlib for `kern_add` (plain name present = C-linkable; mangled-only = fence unbuilt).

## Use It (Linux)

Inspect the fence from outside:

```bash
rustc --edition 2021 --crate-type=staticlib code/abi.rs -o /tmp/abi-check.a && nm /tmp/abi-check.a 2>/dev/null | grep -i "kern_add" | head -3
cc -Wall -c code/caller.c -o /tmp/caller-check.o && nm /tmp/caller-check.o | grep -i "kern_add"
```

What this does: shows the plain `kern_add` exported one side, undefined (`U`) on the other — promise and delivery as `nm` letters (P-02/06's alphabet, bilingual edition).

| Lines | Code | Why it exists |
|---|---|---|
| staticlib `nm` | delivery proof | `T kern_add` (defined, unmangled — the linker's target) |
| caller `nm` | promise proof | `U kern_add` (used-elsewhere — resolves only in a single-target link) |

Change X → Y: attempt the actual link (`cc caller.o abi.a`) on this mixed box. Verify: it fails (MSVC-vs-GNU runtimes — the failure that teaches the one-target rule better than any paragraph; Phase 10 links Linux-target Rust + Linux-target C and it just works).

## Ship It

Artifact: `outputs/ffi-fence-check.md` — width table (never `long`/`int` across, always `i32/u32/i64/u64`), `repr(C)`+`no_mangle`+`extern` trio, `nm T/U` check, `panic="abort"` note, one-target rule. The pre-flight for every kernel file that mixes languages.

## Exercises

1. Easy — add `ppid: i32` to both structs, recompute offsets (expect rsp→12... check align: 4+4+4 then u64 needs align 8 → pad! size 24 — measure, don't guess).
2. Medium — expose `pcb_run(*mut PcbFfi)` from Rust (null-check → `Err`-style `-1`), call shape in C, test null + valid (pointers cross with contracts — the syscall-argument pattern from P-03/04, bilingual).
3. Hard — `panic = "abort"` build: `rustc -C panic=abort ...` the staticlib, `nm` for `__rust_start_panic` absence vs normal build (unwinder symbols vanish — measure the dependency you refuse the kernel).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| address | raw pointers cross as addresses; validity is a *contract*, not a type | [address](../../../../glossary/terms.md#address) |
| heap | `Box` can't cross raw (ownership is Rust-only); cross with `into_raw`/`from_raw` pairs | [heap](../../../../glossary/terms.md#heap) |
| PCB | the struct that crosses first (fields both sides read — keep widths fixed) | [pcb](../../../../glossary/terms.md#pcb) |

## Further Reading

- Rustonomicon FFI chapter — ownership across borders (`into_raw`/`from_raw`, the full ceremony).
- `man 1 nm` `T`/`U` (again — bilingual edition now).
- xv6-rust shims (skim) — production `extern "C"` borders with SAFETY comments per function.

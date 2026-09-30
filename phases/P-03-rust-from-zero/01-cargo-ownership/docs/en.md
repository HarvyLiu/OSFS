# Cargo, Ownership, Borrows — Memory Safety Without a Collector

> Every value has one owner. Move it or borrow it. The compiler counts frees so you can't forget.

**Type:** Learn
**Languages:** Rust
**Prerequisites:** 04-malloc-heap
**College ref:** OSTEP Ch.14 (allocators need discipline; Rust enforces it), MIT 6.1810 Rust refs (kernel modules in Rust), xv6-rust file kernel/alloc (ownership preview)
**Time:** ~75 minutes

## Learning Objectives
- Trace moves vs borrows on a `String`/`Vec` using an ownership diagram
- Implement owned builders, `&` readers, and `&mut` writers that compile under borrow rules
- Explain why use-after-move fails *at compile time* and how that erases use-after-free
- Connect ownership to `malloc`/`free` (same lifecycle, compiler-counted) and to `no_std` allocators ahead

## Concept in 60s

![ownership move](../figures/ownership-move.svg)

<!-- source: ../figures/ownership-move.excalidraw — open in excalidraw.com to redraw -->

Three rules: (1) each value has exactly one owner; (2) assigning/moving transfers ownership (old name dies — using it is a *compile* error, not a crash); (3) borrows (`&` shared, `&mut` exclusive) let others touch without owning — either many readers or one writer, never both (data races rejected *before* running). `String`/`Vec` own [heap](../../glossary/terms.md#heap) bytes and free on drop (scope exit = automatic `free`, exactly once). `&str`/`&[T]` are borrowed views (pointer+length, no free). Copy types (`int`-shaped: `i32`, `bool`) duplicate instead of moving. P-01/04's trilogy (leak/double-free/use-after-free) becomes: leaks need explicit effort, double-free impossible, use-after-move doesn't compile.

## Simulate It (host — rustc single file, no Cargo needed yet)

Full program: `code/main.rs`. Build with `rustc --edition 2021` (Cargo arrives in Exercise 3).

```rust
fn build_name(first: &str, last: &str) -> String {
    let mut s = String::from(first);
    s.push(' ');
    s.push_str(last);
    s // move out: ownership returns to caller
}

fn shout(s: &str) -> String {
    s.to_uppercase() // borrows to read, returns a NEW owner
}

fn push_exclaim(s: &mut String) {
    s.push('!');
}

fn main() {
    let name = build_name("ada", "lovelace");
    println!("name={name}");
    let loud = shout(&name); // borrow: name still alive after
    println!("loud={loud} still-have={name}");
    let mut m = String::from("hi");
    push_exclaim(&mut m); // exclusive borrow: nobody else touches m here
    println!("m={m}");
    let v = vec![10, 20, 30, 40];
    let mid = &v[1..3]; // slice view: pointer+length, no copy
    println!("mid={:?} len={}", mid, mid.len());
}
```

What this does: builds an owned `String`, reads it shared, mutates it exclusive, and views a vector without copying — the four ownership verbs in one run.

| Lines | Code | Why it exists |
|---|---|---|
| 1–6 | `build_name(...) -> String` | `&str` in (borrowed views), `String` out (fresh owner); `s` *moves* to caller (no copy, no free-yet — drop happens in `main`) |
| 8–10 | `shout(&str)` | shared borrow: many readers allowed; returns new owner (input untouched, output owned — the pure-function shape) |
| 12–14 | `&mut String` | exclusive borrow: compiler guarantees no other live borrow of `m` here (the data-race killer, enforced at compile time) |
| 18–20 | use-after-borrow | `&name` then `name` again: legal because shared borrows coexist with the owner (contrast: moving `name` would kill it) |
| 22–23 | `&mut m` call | one writer, zero readers in this statement (try adding a second `&m` print in the same line — refused, then believed) |
| 25–26 | `&v[1..3]` | slice = (ptr,len) view: `{:?}` debug-prints, `.len()` proves no copy (length 2 of a 4-vector) |

Change X → Y: change `shout(&name)` to `shout(name)` (move instead of borrow), keep the next line using `name`. Verify: compile *fails* `use of moved value` (the exact error that replaces use-after-free — read it once, recognize forever; revert to keep building).

## Build It

```bash
make run
make test
```

What this does: compiles with warnings-as-errors equivalent (`-D warnings` denies lints), runs the verbs, then runs 4 inline `#[test]` checks via `rustc --test`.

| Lines | Code | Why it exists |
|---|---|---|
| `rustc --edition 2021` | pinned edition | 2021 semantics (e.g. `println!("{name}")` inline args); matches CI + Docker toolchain |
| `-D warnings` | lints-as-errors | unused `mut`, dead code fail here (the `-Wall -Werror` you already respect, Rust flavor) |
| `--test` | test harness | compiles `#[test]` fns with asserts into a runner (no framework, stdlib only per AGENTS) |

Change X → Y: remove `mut` from `let mut m`. Verify: `cannot borrow as mutable` error naming the line (proves `mut` is a *contract* the borrow checker reads, not a hint).

## Use It (Linux)

Rust's promises are checkable without running anything:

```bash
rustc --version
rustc --edition 2021 --test code/main.rs -o /tmp/rs-test -D warnings && /tmp/rs-test
grep -c "mut\|&" code/main.rs
```

What this does: pins the toolchain, runs the real test binary, and counts borrow syntax (density of `&` = how much sharing this lesson negotiates).

| Lines | Code | Why it exists |
|---|---|---|
| `rustc --test` | harness build | same command `make test` wraps (run it raw once so the wrapper never mystifies) |
| `grep -c` | shape census | high `&` count = borrow-heavy code (kernels read this way — sharing without owning) |

Change X → Y: `rustc --explain E0382` (use-of-moved-value code). Verify: the compiler *teaches* the rule with examples (the error messages are the course's co-tutor — read them fully, always).

## Ship It

Artifact: `outputs/ownership-card.md` — one-owner/move-or-borrow/many-or-one rule trio, `String` vs `&str` / `Vec` vs `&[T]` map, `mut` contract, `--explain` verb. Reuse in every Rust lesson; it *is* the borrow checker on paper.

## Exercises

1. Easy — make `double_all(v: &mut Vec<i32>)`, verify in place (exclusive borrow writes, owner keeps living).
2. Medium — write `first_word(s: &str) -> &str` returning a subslice tied to input lifetime (elision does the paperwork; break it with two inputs to *see* lifetimes demanded).
3. Hard — `cargo init --bin vecdemo && cp code/main.rs vecdemo/src/main.rs && cd vecdemo && cargo test` (Cargo arrives: same tests, real project layout — compare `rustc --test` vs `cargo test` output line by line).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| heap | `String`/`Vec` bytes live here; drop = compiler-inserted free | ../../glossary/terms.md#heap |
| pointer | `&T`/`&mut T` and slices are (ptr,len) views — no ownership, no free | ../../glossary/terms.md#pointer |
| address | borrows compile to addresses with lifetimes; moves compile to copies of the pointer | ../../glossary/terms.md#address |

## Further Reading

- The Rust Book Ch.4 — ownership/borrows/slices (the canonical telling; ours is the OS-flavored abridgment).
- `rustc --explain E0382,E0502,E0505` — moved-value + borrow-conflict lessons from the compiler itself.
- OSTEP Ch.14 re-read — every allocator bug listed there is one borrow rule away from impossible.

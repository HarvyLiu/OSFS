# Structs, Enums, match, Option/Result — Data That Refuses to Be Wrong

> C structs hold. Rust enums *mean*. match makes you handle every case, or it won't compile.

**Type:** Learn
**Languages:** Rust
**Prerequisites:** 01-cargo-ownership
**College ref:** OSTEP Ch.4 (PCB states need enums), MIT 6.1810 error-handling notes, xv6-rust `proc::State` (this lesson's shape, production-hardened)
**Time:** ~75 minutes

## Learning Objectives
- Trace a PCB struct with a state enum through spawn → run → zombie → reap in safe Rust
- Implement `match` over all variants plus `Option` find and `Result` alloc-failure paths
- Explain why `match` must be exhaustive and how that erases forgotten-state bugs
- Connect `Result` to C's `-1/NULL` conventions (same honesty, compiler-checked)

## Concept in 60s

![enum match](../figures/enum-match.svg)

<!-- source: ../figures/enum-match.excalidraw — open in excalidraw.com to redraw -->

A C `struct` + `int state` lets `state = 99` compile (garbage states are representable). A Rust `enum State { Unused, Runnable, Running, Zombie }` makes *only* those four exist — `99` is unspellable. `match` must list every variant (skip one → compile error: the forgotten-case killer). `Option<Pcb>` = maybe-a-row (`Some`/`None` instead of NULL+dereference-roulette). `Result<T, E>` = value-or-named-error instead of `-1` + errno global. Combined: illegal states unrepresentable, missing cases uncompilable, absent rows explicit. The P-01/05 PCB table, rebuilt so three bug classes can't typecheck.

## Simulate It (host rustc, no Cargo needed)

Full program: `code/main.rs`. Four slots, spawn two, run one, zombie one, reap it — P-01/05's story, enforced.

```rust
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
enum State {
    Unused,
    Runnable,
    Running,
    Zombie,
}

#[derive(Clone, Debug)]
struct Pcb {
    pid: i32,
    state: State,
    name: String,
}

struct Table {
    slots: [Pcb; 4],
}

impl Table {
    fn new() -> Self {
        let blank = || Pcb { pid: 0, state: State::Unused, name: String::new() };
        Table { slots: [blank(), blank(), blank(), blank()] }
    }

    fn alloc(&mut self, pid: i32, name: &str) -> Result<usize, &'static str> {
        for (i, s) in self.slots.iter_mut().enumerate() {
            if s.state == State::Unused {
                *s = Pcb { pid, state: State::Runnable, name: name.to_string() };
                return Ok(i);
            }
        }
        Err("table full")
    }

    fn find(&self, pid: i32) -> Option<usize> {
        self.slots
            .iter()
            .position(|s| s.state != State::Unused && s.pid == pid)
    }

    fn describe(&self, i: usize) -> &'static str {
        match self.slots[i].state {
            State::Unused => "free",
            State::Runnable => "ready",
            State::Running => "on-cpu",
            State::Zombie => "dead-awaiting-reap",
        }
    }
}

fn main() {
    let mut t = Table::new();
    let a = t.alloc(1, "init").expect("slot for init");
    let b = t.alloc(2, "shell").expect("slot for shell");
    t.slots[a].state = State::Running;
    t.slots[b].state = State::Zombie;
    println!("{}({}) is {}, {}({}) is {}", t.slots[a].pid, t.slots[a].name, t.describe(a), t.slots[b].pid, t.slots[b].name, t.describe(b));
    let f = t.find(2).expect("shell present");
    t.slots[f].state = State::Unused;
    println!("reaped pid 2; find now: {:?}", t.find(2));
    println!("alloc 5 more: {:?}", t.alloc(3, "x").and(t.alloc(4, "y")).and(t.alloc(5, "z")).and(t.alloc(6, "w")).and(t.alloc(7, "v")));
}
```

What this does: owns the full lifecycle with honesty at every step — allocation can fail (`Result`), lookup can miss (`Option`), states are named (`match`).

| Lines | Code | Why it exists |
|---|---|---|
| 1–7 | `enum State` + derives | four spellable states; `PartialEq` enables `==`, `Debug` enables `{:?}` prints (derives = compiler-written impls, not magic) |
| 9–14 | `struct Pcb` | owned `String` name (no lifetime annotations needed — ownership handles it, unlike borrowed views) |
| `new()` | `blank` closure | builds one free row per call (arrays need `Copy` for `[x; 4]`, and `Pcb` isn't `Copy` due to `String` — four calls instead) |
| 24–35 | `alloc -> Result` | `Ok(i)` slot or `Err("table full")`: failure is a *value* the caller must handle (`.expect` here, `?` in real code — never a silent -1) |
| 37–41 | `find -> Option` | `position` returns `Some(i)`/`None`: absence explicit (C returned NULL and prayed) |
| 43–51 | `match` exhaustive | four arms, no wildcard: add a fifth state later and this *fails to compile until updated* (the forgotten-case killer, demonstrated in Exercise 2) |
| 57–64 | lifecycle | spawn → run/zombie → describe → reap → miss-proven-`None` → fill-to-`Err` (every verb exercised, every failure shown succeeding-at-failing) |

Change X → Y: delete the `State::Zombie` arm. Verify: compiler errors listing the missing pattern (the guarantee is mechanical, not cultural — no linter, no review needed, just `rustc`).

## Build It

```bash
make run
make test
```

What this does: denies warnings, runs the lifecycle, then runs 5 inline `#[test]` checks (alloc/find/reap/full/exhaustive-describe).

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | expect `1 is on-cpu, 2 is dead-awaiting-reap`, `find now: None`, final `Err("table full")` |
| `make test` | machine proof | `rustc --test` harness on the same file (no framework, stdlib only) |

Change X → Y: change `N` slots 4→2 mentally (edit the array + test). Verify: `Err` arrives after 2 allocs (capacity is data, not folklore — tests pin it).

## Use It (Linux)

No new syscalls — but the same honesty appears in errno-style APIs done right:

```bash
./build/pcb-rs
rustc --explain E0004 2>/dev/null | head -8 || rustc --explain E0005 | head -8
```

What this does: runs the lifecycle, then asks the compiler to teach non-exhaustive match (its own error index as tutor, as in P-03/01).

| Lines | Code | Why it exists |
|---|---|---|
| `--explain` | compiler lecture | exact rule + fix examples for the error Exercise 2 manufactures on purpose |

Change X → Y: `cargo new --bin pcbq && cp code/main.rs pcbq/src/main.rs && cd pcbq && cargo test` (Exercise-3-style graduation). Verify: same 5 tests green under Cargo (single-file `rustc` and Cargo agree — workflow swap, zero semantic drift).

## Ship It

Artifact: `outputs/rust-pcb.rs` — the `State`/`Pcb`/`Table` trio as a starter for Phase 10's Rust allocator/process modules (swap `String` for fixed `[u8; 16]` + `no_std` later — the shape survives, the heap type changes).

## Exercises

1. Easy — add `ppid: i32`, set shell's parent to 1, print both via `find` (tree links, as in P-01/05's Exercise 1, now `Option`-safe).
2. Medium — add `State::Sleeping`, compile, fix every `match` the compiler lists (count them: that's your forgotten-case surface, now measured).
3. Hard — replace `&'static str` errors with a real `enum AllocErr { Full }` + `Display` impl; rewrite the final line to print it (errors become data with behavior — the `Result` endgame).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| heap | `String` bytes live here; moved/freed by rules, not calls | ../../glossary/terms.md#heap |
| pointer | `&self`/`&mut self` methods borrow the table (no raw addresses in sight) | ../../glossary/terms.md#pointer |
| PCB | this lesson's struct, states now unrepresentable-to-misuse | ../../glossary/terms.md#pcb |

## Further Reading

- Rust Book Ch.5–6 — structs/enums/match (canonical telling; ours is the PCB-flavored cut).
- `rustc --explain E0004,E0502` — exhaustiveness + borrow-conflict lectures.
- xv6-rust `proc` module (skim) — production `State` + table shapes wearing locks.

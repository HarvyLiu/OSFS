# Traits, Generics, Vec/Box — Shared Behavior, Any Type

> Traits say what you can do. Generics say with what. Box owns the heap when size won't sit still.

**Type:** Learn
**Languages:** Rust
**Prerequisites:** 02-structs-enums
**College ref:** OSTEP Ch.14 (allocators behind `Vec`), MIT 6.1810 trait-object notes (scheduler queues preview), Rust Book Ch.10 (the canonical telling)
**Time:** ~75 minutes

## Learning Objectives
- Trace one `Describe` trait across two structs plus a generic `first()` over any slice
- Implement `Box` heap ownership and `Vec` growth that echoes P-01/04's doubling
- Explain monomorphization (generics stamp per-type copies) vs trait objects (one door, many rooms)
- Connect `Display`/`Drop` to kernel logging and cleanup paths ahead

## Concept in 60s

![traits generics](../figures/traits-generics.svg)

<!-- source: ../figures/traits-generics.excalidraw — open in excalidraw.com to redraw -->

A trait is a promise set (`fn describe(&self) -> String`); any struct can keep it, differently. A generic function (`fn first<T>(s: &[T]) -> Option<&T>`) works for *all* `T` — the compiler stamps a copy per type used (monomorphization: no runtime cost, bigger binary). `Vec<T>` owns a growable [heap](../../glossary/terms.md#heap) array (P-01/04's doubling, compiler-counted). `Box<T>` owns one heap value of unknown-at-compile size (trait objects, linked lists — anywhere `size_of` can't finish). `Display` controls `{}` printing; `Drop` runs at scope end (the automatic `free` from P-03/01, now custom).

## Simulate It (host rustc, no Cargo needed)

Full program: `code/main.rs`. Two welcomers, one generic, one boxed.

```rust
use std::fmt::Display;

trait Describe {
    fn describe(&self) -> String;
}

struct Pcb {
    pid: i32,
    name: String,
}

struct Job {
    id: i32,
    burst: i32,
}

impl Describe for Pcb {
    fn describe(&self) -> String {
        format!("pcb {} ({})", self.pid, self.name)
    }
}

impl Describe for Job {
    fn describe(&self) -> String {
        format!("job {} burst {}", self.id, self.burst)
    }
}

fn first<T>(s: &[T]) -> Option<&T> {
    if s.is_empty() {
        None
    } else {
        Some(&s[0])
    }
}

fn announce<T: Describe>(t: &T) {
    println!("announce: {}", t.describe());
}

fn main() {
    let p = Pcb { pid: 1, name: "init".to_string() };
    let j = Job { id: 7, burst: 3 };
    announce(&p);
    announce(&j);
    let v = vec![10, 20, 30];
    println!("first={:?} len={}", first(&v), v.len());
    let boxed: Box<Pcb> = Box::new(p);
    println!("boxed pid={}", boxed.pid);
    let nums: Vec<String> = Vec::new();
    println!("empty display: '{}' len={}", nums.len(), nums.len());
    print_with_display(&j);
}

fn print_with_display<T: Display>(t: &T) {
    println!("display: {t}");
}

impl Display for Job {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "Job#{}(burst {})", self.id, self.burst)
    }
}
```

What this does: two types keep one promise, one generic reads any slice, one box moves a struct to the heap, one `Display` unlocks `{}` — shared behavior without inheritance.

| Lines | Code | Why it exists |
|---|---|---|
| 3–5 | `trait Describe` | promise set: anything with `.describe()` qualifies (C++ virtuals without the vtable-by-default; Rust picks static dispatch unless asked) |
| 20–30 | two `impl` blocks | same promise, different sentences (the polymorphism kernels use for schedulers/filesystems: one queue, many policies) |
| 32–38 | `first<T>` | generic over element type *and* length-agnostic via slice (works for arrays, `Vec`, PCB tables — write once) |
| 40–42 | `announce<T: Describe>` | bounded generic: only promise-keepers accepted (passing an `i32` fails *at the call* with the bound named) |
| 50–51 | `Box::new(p)` | moves `p` to heap; `p` unusable after (move rule from P-03/01 — `boxed.pid` auto-derefs through the pointer) |
| 56–62 | `Display` for `Job` | `{}` printing is a trait, not magic: implement `fmt`, gain every `{}`/`to_string()` downstream |

Change X → Y: call `announce(&42_i32)` (uncomment a scratch line). Verify: `the trait bound i32: Describe is not satisfied` (bounds reject at compile time — the error *is* the feature working).

## Build It

```bash
make run
make test
```

What this does: denies warnings, runs the promises, then runs 5 inline `#[test]` checks (both describes, generic over two types, box move, display string).

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | expect `announce: pcb 1 (init)`, `announce: job 7 burst 3`, `first=Some(10)`, `boxed pid=1`, `display: Job#7(burst 3)` |
| `make test` | machine proof | harness on the same file (stdlib only) |

Change X → Y: remove `: Describe` bound from `announce`. Verify: `cannot find method describe` (bounds carry the *knowledge* that `.describe()` exists — without it the generic body is blind).

## Use It (Linux)

Generics leave fingerprints in binaries:

```bash
./build/traits-rs
nm -C build/traits-rs 2>/dev/null | grep -iE "first|announce" | head -5 || echo "(nm -C demangle unavailable: stripped or minimal toolchain; RUSTC output below still proves)"
rustc --edition 2021 --test code/main.rs -o /tmp/tg-test -D warnings && /tmp/tg-test 2>&1 | tail -3
```

What this does: runs the verbs, hunts monomorphized copies per type (two `first` stamps: one per caller type), and runs the suite raw.

| Lines | Code | Why it exists |
|---|---|---|
| `nm -C` | stamped copies | expect `first::<i32>`-shaped symbols (monomorphization visible: generics cost binary size, not runtime) |
| `--test` raw | wrapperXBypass | same command `make test` hides (run raw once per lesson — wrappers never mystify) |

Change X → Y: add `first(&vec!["a","b"])` call (a `&str` stamp). Verify: new `first::<&str>` symbol appears (each type pays its own copy — the tradeoff trait *objects* avoid at runtime cost).

## Ship It

Artifact: `outputs/traits-card.md` — trait/generic/`Box`/`Display` recipes, bound-error reading guide, monomorphization-vs-objects rule. Reuse when Phase 10's scheduler takes `T: Policy` (you'll recognize your own handwriting).

## Exercises

1. Easy — implement `Describe` for `Vec<Job>` (join lines); announce a 2-job queue (traits compose over containers).
2. Medium — write `largest<T: PartialOrd>(s: &[T]) -> Option<&T>` + tests over `i32` and `char` (second stamp type — watch `nm` gain it).
3. Hard — trait object run: `let announcers: Vec<Box<dyn Describe>> = vec![Box::new(p), Box::new(j)]` loop-announce (one door `dyn`, many rooms; compare binary size vs generics and argue static-vs-dynamic dispatch for a scheduler hot path).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| heap | `Vec`/`Box` bytes live here; traits abstract over them | ../../glossary/terms.md#heap |
| pointer | `&T` borrows, `Box<T>` owns-through-pointer (both deref, only one frees) | ../../glossary/terms.md#pointer |
| PCB | `Pcb` keeps promises now (describe today, schedule tomorrow) | ../../glossary/terms.md#pcb |

## Further Reading

- Rust Book Ch.10 — generics/traits/lifetimes (canonical; ours is the scheduler-flavored cut).
- `rustc --explain E0277` — trait-bound failures, taught by the compiler.
- OSTEP Ch.14 re-read — `Vec`'s doubling is that chapter running with seatbelts.

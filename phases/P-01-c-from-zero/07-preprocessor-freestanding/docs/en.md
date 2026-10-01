# Preprocessor, Freestanding vs Hosted — Code About Code

> `#define` pastes. `#ifdef` forks reality. Freestanding deletes the floor.

**Type:** Learn
**Languages:** C
**Prerequisites:** 06-headers-make
**College ref:** OSTEP Ch.1 (abstractions you will rebuild), MIT 6.1810 freestanding kernel builds, xv6 file kernel/defs.h + `CFLAGS -ffreestanding`
**Time:** ~60 minutes

## Learning Objectives
- Trace `#define`/`#ifdef`/`#include` through `gcc -E` output line by line
- Implement macro-safe min, `static_assert` layout checks, and debug-vs-release forks
- Explain hosted (libc + startup) vs freestanding (you are the runtime) via `-ffreestanding -nostdlib`
- Connect freestanding to Tooling 02's kernel (no `printf`/`malloc` until you write them)

## Concept in 60s

![build modes](../figures/build-modes.svg)

<!-- source: ../figures/build-modes.excalidraw — open in excalidraw.com to redraw -->

The preprocessor runs *before* the compiler: dumb text paste withsharp knives. `#define MIN(a,b)` pastes expressions (parenthesize everything or precedence eats you). `#ifdef DEBUG` compiles two programs from one file. `#include` pastes headers (P-01/06's promises). Hosted C = libc + `_start` + `main` + `exit` provided (your sims so far). Freestanding C = language only: no `printf`, no `malloc`, `main` means nothing until a linker script + your entry say so. Kernels compile `-ffreestanding -nostdlib` and provide UART print + page alloc themselves. See [freestanding](../../../../glossary/terms.md#freestanding).

## Simulate It (host — macros with teeth, no QEMU)

Full program: `code/macros.c`. Safe min, stringize, debug fork.

```c
#include <assert.h>
#include <stdio.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define STR_(x) #x
#define STR(x) STR_(x)

#ifndef NDEBUG
#define LOG(msg) printf("dbg: %s\n", msg)
#else
#define LOG(msg) ((void)0)
#endif

int main(void) {
    int x = 3, y = 4;
    printf("min=%d line=%d file=%s ver=%s\n",
           MIN(x + 1, y), __LINE__, __FILE__, STR(__STDC_VERSION__));
    LOG("visible in debug build");
    static_assert(sizeof(int) >= 2, "int too small for this course");
    return 0;
}
```

What this does: exercises paste (`MIN`), stringize (`STR`), compile-time forks (`LOG`), location builtins, and a layout guarantee — the preprocessor toolkit every kernel header uses.

| Lines | Code | Why it exists |
|---|---|---|
| 3 | `MIN` fully parenthesized | `MIN(x+1,y)` pastes `((x+1)<(y)?...)`; drop inner parens and `x+1` binds wrong (the classic macro bug — Exercise 1 removes them to watch it break) |
| 4–5 | two-step `STR` | `#x` stringizes *after* expansion only through the helper (`STR(VERSION)` → `"201112L"`, single-step would print `"__STDC_VERSION__"`) |
| 7–11 | `LOG` fork | debug prints, release compiles to nothing (`-DNDEBUG` flips reality without touching code) |
| 15–16 | `__LINE__/__FILE__` | paste location: assert messages + logs locate themselves (kernels stamp panics this way) |
| 17 | `static_assert` | compile-time contract: layout wrong = build fails here, not a 3 AM page fault later |

Change X → Y: compile with `-DNDEBUG`, rerun. Verify: `dbg:` line vanishes (proves the fork is compile-time — the binary literally lacks the call; check with `strings`).

See the paste with your own eyes:

```bash
gcc -E code/macros.c | grep -A2 "int main" | head -8
gcc -DNDEBUG -E code/macros.c | grep -c "dbg:"
```

What this does: expands all directives to raw C, then counts debug remnants in the release flavor (expect 0).

| Lines | Code | Why it exists |
|---|---|---|
| `gcc -E` | preprocessor only | output = what the compiler *actually* compiles (P-01/06's paste claim, proven) |
| `grep -c "dbg:"` | release audit | `0` = no debug strings ship (binary diet + no info leaks) |

Change X → Y: add `-DSTRANGE` (undefined macro, harmless). Verify: output identical (proves unknown `-D` flags are inert unless `#ifdef` reads them).

## Build It (freestanding — remove the floor, feel the air)

```bash
make run
make freestanding
```

What this does: runs hosted (full floor), then compiles the *same-ish* file freestanding to show what vanishes.

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | hosted proof | expect `min=4 line=... file=... ver=...` + `dbg:` line |
| `make freestanding` | floor removal | `cc -ffreestanding -nostdlib -c bare.c` — compiles (syntax valid!) but linking/running needs Tooling 02's script + entry (that's the whole kernel job) |

`code/bare.c` (what survives without libc):

```c
// bare.c -- freestanding: no headers, no libc, just language.
int add(int a, int b) { return a + b; }
```

What this does: proves pure logic compiles identically hosted or freestanding — only *services* (I/O, memory, startup) disappear, never the language.

| Lines | Code | Why it exists |
|---|---|---|
| no `#include` | nothing to paste | freestanding provides *no* headers except `<stdint.h>`-style freestanding ones (check your `gcc -ffreestanding` docs) |
| `add` only | no `main`, no I/O | entry point comes from `boot.s` + linker script (Tooling 02); output via serial you write (P-02/05's `outb`) |

Change X → Y: add `#include <stdio.h>` to `bare.c`, rerun `make freestanding`. Verify: still compiles (`-c` only pastes declarations) — the failure comes at *link*, not compile (headers promise, libraries deliver — P-01/06 again, now with the library gone).

## Use It (Linux)

Real projects fork reality constantly:

```bash
gcc -dM -E - < /dev/null | grep -E "linux|__x86_64__|__STDC_VERSION__" | head -5
grep -rn "ifndef\|ifdef" phases/P-01-c-from-zero/06-headers-make/code/vec.h
```

What this does: dumps the compiler's *predefined* macros (your invisible `#ifdef` landscape) and revisits our own guard in the wild.

| Lines | Code | Why it exists |
|---|---|---|
| `gcc -dM -E` | predefined paste | `__linux__`, `__x86_64__` exist without you defining them — code forks per platform silently (kernels lean on these) |
| `grep ifndef vec.h` | our guard, again | the same two lines protecting every lesson since P-01/06 (now you read them as preprocessor, not ritual) |

Change X → Y: `gcc -U__linux__ -dM -E - < /dev/null | grep -c __linux__`. Verify: `0` (proves predefined macros are undef-able — portability shims do exactly this in tests).

## Ship It

Artifact: `outputs/pp-card.md` — parenthesize-everything, two-step stringize, guard pattern, `-E`/`-dM`/`-DNDEBUG` verbs, freestanding flag set. Reuse every time a header misbehaves (it will).

## Exercises

1. Easy — de-parenthesize MIN to `#define BAD(a,b) a<b?a:b`, call `BAD(x+1,y)*10`, show the wrong answer with `-E` output beside it (precedence made visible).
2. Medium — add `#ifdef VERBOSE` extra prints to `macros.c`; build both flavors; `strings` each binary and diff the evidence (release must lack the strings).
3. Hard — write `bare_main` in `bare.c` + a `mini.ld` placing it at a fixed address, `objdump -d` the `.o`: no `main`, no startup, just your bytes (this *is* Tooling 02's kernel minus serial — compare the two).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| freestanding | language without libc/startup; kernels live here | [freestanding](../../../../glossary/terms.md#freestanding) |
| address | linker (not preprocessor) assigns final addresses post-paste | [address](../../../../glossary/terms.md#address) |
| heap | `malloc` is hosted-only; freestanding allocators are hand-built (Memory phases) | [heap](../../../../glossary/terms.md#heap) |

## Further Reading

- `man 1 gcc` (`-E`, `-dM`, `-D`, `-U`, `-ffreestanding`, `-nostdlib`).
- `man 1 cpp` — the preprocessor's own manual (one page that demystifies all headers).
- xv6 `Makefile` CFLAGS + `kernel/defs.h` — guards + freestanding flags in production arrangement.

# Hello C — Types, Control Flow, Your First Build

> Type it, compile it, run it. Everything else in C is vocabulary on top of this loop.

**Type:** Learn
**Languages:** C
**Prerequisites:** None
**College ref:** OSTEP Ch.1 Dialogue (programs vs OS), MIT 6.1810 C refresher (types/control), xv6 file kernel/printf.c (where printf goes later)
**Time:** ~60 minutes

## Learning Objectives
- Trace `cc main.c -o hello && ./hello` as compile → link → load → run
- Implement types (`int`, `char`, `long`), `if`/`for`/`while`, functions with args and return
- Explain warnings as bugs (`-Wall -Werror`) and exit codes (`return 0`)
- Connect hosted `printf` to the serial print you'll write bare-metal later

## Concept in 60s

![compile pipeline](../figures/compile-pipeline.svg)

<!-- source: ../figures/compile-pipeline.excalidraw — open in excalidraw.com to redraw -->

You write text (`main.c`). The compiler translates it into machine code (`hello`). The OS loads it and runs `main()`. `printf` asks the OS to put bytes on your terminal. `return 0` tells the parent "success" — any other number means "something failed", and parents read it later via `$?` and `wait()`. Types are just "how many bytes, and how to read them": `char` is 1, `int` is usually 4, addresses come later. No [heap](../../../../glossary/terms.md#heap) yet. No [pointers](../../../../glossary/terms.md#pointer) yet. Just the loop.

## Simulate It (host, no QEMU)

Your simulator is plain hosted C. `code/main.c` runs on Linux, WSL, Docker, or Windows — no QEMU needed.

```c
#include <stdio.h>

int square(int x) {
    return x * x;
}

int main(void) {
    for (int i = 0; i < 5; i++) {
        if (i % 2 == 0)
            printf("i=%d square=%d (even)\n", i, square(i));
        else
            printf("i=%d square=%d\n", i, square(i));
    }
    return 0;
}
```

What this does: defines a helper, loops 0–4, branches on even/odd, prints each square — the four ideas (function, loop, branch, I/O) every later simulator reuses.

| Lines | Code | Why it exists |
|---|---|---|
| 1 | `#include <stdio.h>` | declares `printf`; hosted-only (freestanding kernels reimplement output) |
| 3–5 | `int square(int x)` | function: takes int, returns int; OS-why: every helper (e.g. `alloc`, `schedule`) has this shape |
| 7 | `int main(void)` | OS entry: runs after libc startup, return value becomes process exit code |
| 8 | `for (int i=0; i<5; i++)` | loop 5 times; `int i` lives on the [stack](../../../../glossary/terms.md#stack), dies at `}` |
| 9 | `if (i % 2 == 0)` | `%` = remainder; `==` = compare (single `=` would assign — classic beginner bug `-Wall` catches) |
| 10–12 | `printf(...)` | formatted print: `%d` = decimal int; `\n` = newline + flush line |
| 14 | `return 0;` | exit code 0 = success; parent reads via `$?` or `wait()` (Processes phase) |

Change X → Y: change `i < 5` to `i < 3`. Verify: `make run` prints 3 lines ending `i=2` (proves the loop bound drives output, nothing hardcoded).

Types you must know cold (all in `code/types.c`, run via `make types`):

```c
#include <stdio.h>
int main(void) {
    char c = 'A';
    int n = -42;
    long big = 1000000L;
    printf("c=%c (%d) n=%d big=%ld sizes=%zu/%zu/%zu\n",
           c, c, n, big, sizeof(c), sizeof(n), sizeof(big));
    return 0;
}
```

What this does: prints one of each core type plus its byte size on *your* machine, so `sizeof` stops being folklore.

| Lines | Code | Why it exists |
|---|---|---|
| 3–5 | `char/int/long` | 1 / usually-4 / 8-or-4 bytes; `char` prints as letter with `%c`, number with `%d` (same byte, two views) |
| 6–7 | `sizeof(...)` + `%zu` | `sizeof` = bytes at compile time; `%zu` = correct print for `size_t` (using `%d` warns — good, warnings are bugs) |
| `1000000L` | `L` suffix | forces `long` literal; without it, big constants can overflow `int` silently |

Change X → Y: change `'A'` to `'a'`. Verify: `make types` shows `97` not `65` (proves `char` is just a small number — ASCII table preview, used in P-00).

## Build It (the compile itself is the build)

```bash
cc -Wall -Werror -std=c11 main.c -o hello && ./hello; echo "exit=$?"
```

What this does: compiles with warnings-as-errors, runs, then shows the exit code — the exact loop for every host lesson.

| Lines | Code | Why it exists |
|---|---|---|
| `cc -Wall -Werror` | all warnings = errors | catches `=` vs `==`, missing returns, wrong `%` verbs before they become runtime bugs |
| `-std=c11` | language version | pins behavior (e.g. `for (int i...)` declarations); matches CI + Docker |
| `-o hello` | output name | without it you get `a.out` (fine, but naming beats archaeology) |
| `./hello; echo "exit=$?"` | run + proof | `./` = "this dir, not PATH"; `$?` = exit code from `return 0` |

Change X → Y: delete one `;` and rebuild. Verify: compiler points at the exact line (read the error top-down; first error is real, rest are echoes — debugging mindset P-00/04).

You will type those flags once, then let `make` remember them:

```make
run:
	cc -Wall -Werror -std=c11 main.c -o build/hello && ./build/hello
```

What this does: identical compile+run, one word (`make run`), flags versioned in git instead of shell history.

## Use It (Linux)

Step outside your program and watch the OS see the same loop:

```bash
./build/hello
echo $?
strace -e trace=write ./build/hello 2>&1 | head -8
```

What this does: runs your binary, shows its exit code, then reveals the `write` [syscall](../../../../glossary/terms.md#syscall) hiding inside every `printf`.

| Lines | Code | Why it exists |
|---|---|---|
| `./build/hello` | run | your bytes, loaded by Linux's loader (preview of exec) |
| `echo $?` | exit code | `0` from `return 0`; change to `return 3` → `3` (the parent-observable contract) |
| `strace -e trace=write` | trap trace | each `printf` line = one `write(1, ...)` to fd 1 (stdout); OS-why: `printf` is just formatting + this trap |

Change X → Y: replace `write` with `process` (`-e trace=process`). Verify: nearly empty — proves printing uses `write`, not process management (syscalls have families; you'll meet each in its phase).

## Ship It

Artifact: `outputs/c-loop-card.md` — the 4-line loop (edit → `make run` → read first error → verify `$?`). Tape it to your monitor; it stays valid through capstone, and you will repeat it until it feels automatic.

## Exercises

1. Easy — make it print squares 0–9 with `odd`/`even` labels. Show output + exit code.
2. Medium — add `int cube(int x)` and print both. Turn on `-Wextra`, fix any new warning.
3. Hard — return the count of even numbers as exit code (`return evens;`), then `echo $?` proves parents can read computed results (preview of `wait()`).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| stack | where `i`, `x` live; freed at `}` | [stack](../../../../glossary/terms.md#stack) |
| syscall | `printf` → `write` trap underneath | [syscall](../../../../glossary/terms.md#syscall) |
| freestanding | kernel C without `printf`; coming in Tooling 02 | [freestanding](../../../../glossary/terms.md#freestanding) |

## Further Reading

- `man 3 printf` — every `%` verb we used, plus the ones we'll need (`%p`, `%x` in P-00).
- `man 1 cc` — what `-Wall -Werror -std` actually request.

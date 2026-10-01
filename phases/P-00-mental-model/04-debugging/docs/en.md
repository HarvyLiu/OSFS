# Debugging — Read the Crash, It Read You First

> Two bugs, one crash, zero mysteries: the compiler warns, asserts pin, gdb points at the exact line.

**Type:** Learn
**Languages:** C
**Prerequisites:** 03-memory-map
**College ref:** OSTEP Ch.2 (no debugging chapter — every chapter assumes it), MIT 6.1810 gdb guide, `man gdb`
**Time:** ~60 minutes

## Learning Objectives
- Reproduce a crash and a silent wrong answer from the same 30-line program
- Read a `gdb` backtrace (`run` → `bt` → guilty line) on Linux or Windows
- Explain why `NULL` unchecked and `<=` instead of `<` are a crash and a corruption respectively
- Fix both, pin with asserts, exit 0

## Concept in 60s

![debug loop](../figures/debug-loop.svg)

<!-- source: ../figures/debug-loop.excalidraw — open in excalidraw.com to redraw -->

You will meet this moment: one input kills the program, another does not, and the code looks innocent. Work the loop with four stations. **Reproduce** with the same input and the same death ("bob" kills, "amy" doesn't). **Observe** through compiler warnings, prints, and `gdb`. **Hypothesize** about which [pointer](../../../../glossary/terms.md#pointer) is NULL or which bound is off. **Pin** it with an `assert` that fails before the fix and passes after. Beginners skip station one and guess. Don't guess — the machine will tell you the line number for free.

Our patient `code/buggy.c` carries two classic diseases. BUG 1: `lookup` returns `NULL` for unknown names and `main` uses it unchecked, so the program crashes. BUG 2: `average` loops `i <= n`, one past the end, and reads whatever lives next door (P-00/03's stack neighbor). One kills loudly and one corrupts silently. Guess which one is scarier.

## Simulate It (host — meet the patient)

```bash
gcc -Wall -Werror -Wextra -std=c11 -g buggy.c -o buggy
./buggy amy
./buggy amy
./buggy bob; echo "exit=$?"
```

What this does: builds with debug symbols (`-g` — names and line numbers survive into the binary), runs the good input twice, then the killer input.

| Lines | Code | Why it exists |
|---|---|---|
| `-g` | debug info | without it gdb shows addresses, not lines (`buggy.c:21` vs `0x41186d` — pay the bytes, keep the names) |
| `./buggy amy` twice | nondeterminism demo | the average differs run to run (overread garbage changes — silent corruption never repeats nicely) |
| `echo exit=$?` | contract | nonzero = death certificate (139/`SIGSEGV` Linux, `0xC0000005` Windows — same crime, different precinct) |

Expected output (numbers vary — *that's the point*):

```text
id=1001 (len=4)
avg=117119766.7
id=1001 (len=4)
avg=-204072169.3
bob-exit=-1073741819
```

What this does: proves BUG 2 (two runs, two insane averages — the loop reads past the array into stack garbage) and BUG 1 (bob never prints — dead before the average).

Change X → Y: `./buggy amy` → `./buggy` (no args). Verify: `usage: buggy <name>`, exit 1 (the one path the author *did* guard — argc checked, return value unchecked; authors guard what they've been burned by).

## Build It (gdb points at the line)

```bash
gdb -batch -ex run -ex bt --args ./buggy bob
```

What this does: runs the killer input under the debugger, prints the backtrace at death — no interaction needed (`-batch` = scripted gdb, CI-friendly).

| Lines | Code | Why it exists |
|---|---|---|
| `-ex run` | execute | starts the program with following args (`--args ./buggy bob` — the reproducer, station one, automated) |
| `-ex bt` | backtrace | prints the call [stack](../../../../glossary/terms.md#stack) at the crash (P-02/02's frames, now as a crime scene) |

Expected output:

```text
Thread 1 received signal SIGSEGV, Segmentation fault.
0x... in strlen () from .../ucrtbase.dll (or libc.so.6 on Linux)
#0  ... in strlen ()
#1  ... in main (argc=2, argv=...) at buggy.c:21
```

What this does: names the killer — frame 0 died inside `strlen`, frame 1 shows *your* line called it with what must be NULL (only NULL kills `strlen` — hypothesis writes itself).

| Lines | Code | Why it exists |
|---|---|---|
| frame 0 | library death | crash inside libc/ucrt, not your code (library functions don't check NULL — the *caller* must, by contract) |
| frame 1 `buggy.c:21` | your line | `strlen(id)` with `id == NULL` from `lookup("bob")` → `return 0` (follow the NULL backwards: use → variable → function — three hops, one minute) |

Change X → Y: `--args ./buggy bob` → `--args ./buggy amy`. Verify: exits normally, no signal (proves the debugger setup works and amy truly never touches the bug — reproducers must discriminate).

The fix (`code/fixed.c`): check the pointer, bound the loop:

```c
const char *id = lookup(argv[1]);
if (!id) { printf("unknown name: %s\n", argv[1]); return 2; }
```

What this does: turns a crash into a message plus a distinct exit code (2 = "your input", not 1 = "your usage" — callers can tell them apart, P-00/02's contract thinking).

| Lines | Code | Why it exists |
|---|---|---|
| `if (!id)` | NULL check | every nullable return gets one at the use site (the rule is positional: check where you *use*, not where it's *born*) |
| `return 2` | new code | usage error (1) vs unknown name (2) — scripts branching on `$?` thank you later |

```c
for (int i = 0; i < n; i++) sum += a[i];
```

What this does: visits exactly `a[0..n-1]` — `<` means "n elements", `<=` means "n+1 hopes".

| Lines | Code | Why it exists |
|---|---|---|
| `<` not `<=` | bound | C arrays are 0-based: valid indices are `0..n-1` (the single most-typed wrong character in C — count on it, then count again) |

## Use It (Linux — and Windows)

You run the same session on Linux or Windows — gdb ships with MinGW too:

```bash
make
./fixed amy
./fixed bob; echo "exit=$?"
make test
```

What this does: builds both binaries warning-clean, shows the cure on both inputs, runs the assert suite.

| Lines | Code | Why it exists |
|---|---|---|
| `./fixed bob` exit 2 | message not crash | `unknown name: bob` (BUG 1: pinned — death became diagnostics) |
| `avg=80.0` | exact | (90+80+70)/3, twice in a row (BUG 2: pinned — chaos became arithmetic) |

Change X → Y: delete the `if (!id)` line, `make test` still passes but `./fixed bob` crashes. Verify: tests pin the *library* (`lookup` returns NULL — correct!), not the *program's* handling (the suite guards functions; gdb guards `main` — two nets, different holes).

## Ship It

Artifact: `outputs/debug-card.md` — reproduce → observe → hypothesize → pin; `-g` always; `gdb -batch -ex run -ex bt`; check nullable returns at use; `<` for n elements; distinct exit codes. Tape it to the monitor — every phase after this assumes you can run it cold.

## Exercises

1. Easy — plant a third bug (divide the average by `n+1`), watch the test fail, un-plant it (feel the net catch).
2. Medium — `gdb -ex "break average" -ex run -ex "print a[0]@4" --args ./buggy amy` (inspect the array *and* its garbage neighbor — see BUG 2's extra element with your own eyes).
3. Hard — add `assert(id)` *inside* `lookup`'s callers vs at the return: argue which position catches more (defense in depth has a cost — where's the cheapest checkpoint?).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| pointer | `id` is NULL-able — the type doesn't say so, the function does | [pointer](../../../../glossary/terms.md#pointer) |
| stack | the backtrace *is* the stack, frozen at death (frames with names) | [stack](../../../../glossary/terms.md#stack) |
| address | `strlen(NULL)` reads address 0 — unmapped by design, crash by design | [address](../../../../glossary/terms.md#address) |
| syscall | exit codes are the process's last syscall (`exit_group(2)` — scripts read it) | [syscall](../../../../glossary/terms.md#syscall) |

## Further Reading

- MIT 6.1810 gdb guide (the five commands that solve 90%: run, bt, break, print, continue).
- `man gdb` batch mode (script your debugging — reproducers belong in CI, not memory).
- OSTEP Ch.2 (processes die with signals — this lesson's crash, formalized).

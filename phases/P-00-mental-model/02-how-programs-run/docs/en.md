# How Programs Run — Preprocess, Compile, Assemble, Link, Load

> Text in, process out. Four tools touch your code before it ever runs — meet each one by name.

**Type:** Learn
**Languages:** C
**Prerequisites:** 01-bits-binary-hex
**College ref:** OSTEP Ch.1 (how the OS runs programs), Bryant & O'Hallaron Ch.7 (linking, preview), `man 1 gcc` (the stage flags)
**Time:** ~60 minutes

## Learning Objectives
- Trace `hello.c` through `-E` (paste), `-S` (assembly), `-c` (object), link (binary), `exec` (process)
- Implement an argv-echo program and read its exit code as parent-visible data
- Explain why each stage exists (and which error belongs to which stage)
- Connect stages to later lessons (headers P-01/06, ASM P-02, linker P-02/06, loader 02-fork-exec)

## Concept in 60s

![pipeline stages](../figures/pipeline-stages.svg)

<!-- source: ../figures/pipeline-stages.excalidraw — open in excalidraw.com to redraw -->

`cc hello.c -o hello` hides four steps: **preprocess** (`#include` paste, `#define` expand → translation unit), **compile** (C → assembly `.s`), **assemble** (assembly → object `.o`: machine code + unresolved symbols), **link** (objects + libraries → executable with [addresses](../../../../glossary/terms.md#address) resolved). Then the **loader** (`exec`) maps the binary into a fresh [address](../../../../glossary/terms.md#address) space and jumps to `_start` → `main(argc, argv)`. Errors name their stage: `error: stdio.h: No such file` (preprocess), `expected ';'` (compile), `undefined reference` (link), `No such file` at run (loader). Read the *first* error: later stages echo earlier breakage.

## Simulate It (host C — argv/echo/exit, the runtime half)

Full program: `code/args.c`. The loader calls *this* shape every time.

```c
#include <stdio.h>

int main(int argc, char **argv) {
    printf("argc=%d\n", argc);
    for (int i = 0; i < argc; i++)
        printf("argv[%d]=%s\n", i, argv[i]);
    return argc - 1;
}
```

What this does: prints its own invocation (count + strings) and exits with `argc-1` — the parent-observable contract from P-01/01, now with arguments.

| Lines | Code | Why it exists |
|---|---|---|
| 3 | `argc/argv` | loader-built: count + string array (`argv[0]` = how *you* were invoked — path included, try `./` vs absolute) |
| 4–6 | echo loop | proves args arrive as C strings (P-01/02's zero-terminated citizens, delivered by `exec`) |
| 7 | `return argc-1` | exit code = data (no args → 0/success; 3 args → 2 — parents branch on this, shells print `$?`) |

Change X → Y: run with 0 vs 4 extra args. Verify: `argc=` tracks, exit code follows (`echo $?` — the shell reading your return, the way `wait()` will in 02-fork-exec).

## Build It (watch each stage emit its artifact)

```bash
cc -E code/hello.c -o build/hello.i && wc -l code/hello.c build/hello.i
cc -S code/hello.c -o build/hello.s && head -8 build/hello.s
cc -c code/hello.c -o build/hello.o && nm build/hello.o | grep -E " main$"
cc build/hello.o -o build/hello && ./build/hello; echo "exit=$?"
```

What this does: runs the four stages separately (800-line paste → assembly → object → runnable), proving each tool's output exists.

| Lines | Code | Why it exists |
|---|---|---|
| `-E` + `wc` | paste audit | ~10 lines → 800+ (`stdio.h` pasted — P-01/07's claim, counted) |
| `-S` + `head` | assembly peek | `.globl main`, `movl`, `call printf` (P-02's dialect, compiler-written — compare with your handwritten `add.s`) |
| `-c` + `nm` | object proof | `T main` + `U printf` (P-01/06's letters: defined here, resolved at link) |
| link + run | loader proof | exit code printed (the process lived and reported — all four stages + load, six commands) |

Change X → Y: delete `#include <stdio.h>`, rerun `-E` then full build. Verify: `-E` fine, build warns `implicit declaration of printf` (stage isolation: paste never checks *meaning* — compile does).

`code/hello.c` (the patient):

```c
// hello.c -- the four-stage patient. Lesson docs/en.md.
#include <stdio.h>

int main(void) {
    printf("hello stages\n");
    return 0;
}
```

What this does: minimal hosted program — small enough that every stage's output stays readable.

## Use It (Linux)

Stages leave distinct error signatures — learn to triage by stage:

```bash
echo 'int main(void) { retrn 0; }' > /tmp/typo.c; cc /tmp/typo.c -o /tmp/typo 2>&1 | head -3
echo 'int main(void);' > /tmp/decl.c; cc -c /tmp/decl.c -o /tmp/decl.o && cc /tmp/decl.o -o /tmp/decl 2>&1 | head -3
file build/hello && ldd build/hello 2>/dev/null | head -3
```

What this does: manufactures a compile error (typo), a link error (declared-never-defined), then inspects linkage (dynamic loader path — the loader's own address).

| Lines | Code | Why it exists |
|---|---|---|
| typo build | compile error | `retrn` unknown *before* `;` (parser stage — fix top-down, first error only) |
| decl link | link error | `undefined reference to main` (compiled fine — delivery missing; P-01/06's sermon, rehearsed) |
| `file + ldd` | loader view | `interpreter /lib64/ld-linux...` (the program that runs your program — loaders all the way down) |

Change X → Y: `cc -static` the hello (if static libc present). Verify: `file` says statically linked, `ldd` refuses (no interpreter — like our kernels: self-contained by construction).

## Ship It

Artifact: `outputs/stages-card.md` — stage/flag/artifact/error-signature table (`-E/-S/-c`, `.i/.s/.o`, paste/compile/link/loader errors). Triage card for every future build failure (read the first error, name its stage, fix there).

## Exercises

1. Easy — run each stage on `args.c`, record line counts (`.c` vs `.i` vs `.s`): paste multiplies, compile translates 1:≈5, assemble 1:1.
2. Medium — `nm` every `.o` in P-01/06 (`vec.o`, `main.o`): list `T` vs `U` per file (who defines, who borrows — the link contract, audited).
3. Hard — `strace -f -e trace=process,openat cc hello.c` and count `execve` children (preprocess/compile/assemble/link are *separate programs* — `cc` is a driver, not a compiler; meet the family).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| address | linker assigns final ones; loader maps them into a fresh space | [address](../../../../glossary/terms.md#address) |
| syscall | `execve` starts the loader (the trap that begins processes) | [syscall](../../../../glossary/terms.md#syscall) |
| freestanding | kernels skip hosted startup (no `_start`-from-libc — own entry instead) | [freestanding](../../../../glossary/terms.md#freestanding) |

## Further Reading

- `man 1 gcc` (OPTIONS: `-E/-S/-c/-o/-static` — the five flags used).
- Bryant & O'Hallaron Ch.7 (linking: symbols, relocation — the deep version when ready).
- OSTEP Ch.1 re-read (now every abstraction names its stage).

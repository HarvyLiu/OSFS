# Lesson Template — copy this folder to start a new lesson

```text
phases/<NN>-phase/<MM>-slug/
  docs/en.md
  code/Makefile + main.c / boot.s / main.rs + tests/
  figures/<diagram>.excalidraw + <diagram>.svg
  quiz.json
  outputs/
```

## docs/en.md skeleton

```markdown
# <Title>

> <One-line hook — what sticks>

**Type:** Learn | Build
**Languages:** C, ASM, Rust (subset actually used)
**Prerequisites:** <slugs or None>
**College ref:** OSTEP Ch.X, MIT 6.1810 Lec Y, xv6 file Z
**Time:** ~<minutes>

## Learning Objectives
- <4-6 verbs: trace, implement, explain...>

## Concept in 60s
![diagram](../figures/<name>.svg)
<!-- source: ../figures/<name>.excalidraw — open in excalidraw.com to redraw -->
1-paragraph intuition. No code yet.

## Simulate It (host, no QEMU)

```c
// code/main.c — minimal runnable slice
```

What this does: <1-2 sentences, plain English>.

| Lines | Code | Why it exists (OS-why) |
|---|---|---|
| ... | ... | ... |

Change X → Y: <one predictable edit>. Verify: `<cmd>` → expected: `<output>`.

## Build It (QEMU / bare-metal)

Same codeblock contract. Document `-ffreestanding`, linker script, `panic_handler` if bare-metal.
ASM uses AT&T GAS. Explain `b/w/l/q`, `%reg`, `$imm`. Side-table Intel ↔ AT&T if new instruction appears.

## Use It (Linux)

```bash
strace -f -e trace=process ./a.out
cat /proc/<pid>/status
```

What this does + line table (yes, even for shell). Connect sim → real kernel.

## Ship It

Artifact saved in `outputs/`: <runbook.md / template / script>. How to reuse in 2 lines.

## Exercises
1. Easy — rerun with one value changed.
2. Medium — apply to new case.
3. Hard — combine with a prior lesson.

## Key Terms
| Term | Plain meaning | Link |
|---|---|---|
| pointer | ... | ../../glossary/terms.md#pointer |

## Further Reading
- OSTEP Ch.X — why worth reading (1 line)
- man 2 <syscall> — ...
```

## quiz.json — 1 pre + 3 check + 2 post, 0-indexed `correct`

## Figures

- Draw in Excalidraw, save `.excalidraw` (source of truth).
- Export `.svg` (and `.dark.svg` if needed), reference from `en.md`.
- Never commit SVG without same-basename `.excalidraw`.

## Code rules

- Host sim exits 0, no hangs, no network.
- Header comment (4-6 lines) cites `docs/en.md`.
- Tests: ≥3 asserts, runnable via `make test` or `cargo test`.
```

> Why AT&T and not Intel? Linux kernel, `objdump`, and `gdb` default to AT&T. Intel appears only as a translation column so you can read both.

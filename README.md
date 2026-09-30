# OS From Scratch (OSFS)

> College OS, rewritten for beginners. Concept first, hands-on second. Graphed, runnable, online.

Learn what an operating system **actually does** — then build one. Full-semester coverage (processes, scheduling, concurrency, memory, persistence, filesystems, protection) plus a parallel **build-your-own-kernel** track that ends with `myos` booting to a shell prompt.

Inspired by the structure of [ai-engineering-from-scratch](https://github.com/rohitg00/ai-engineering-from-scratch): `phases/ → lessons/ → docs + code + figures + quiz + outputs`, with `README.md` + `ROADMAP.md` as source of truth.

## Why this exists

College OS content (OSTEP, MIT 6.1810, xv6) is excellent but dense, text-heavy, and hard to navigate. This repo keeps the rigor and:

- **Teaches code first:** Primer track takes true beginners to OS-ready in C, ASM (GAS AT&T — what Linux itself uses), and Rust.
- **Explains every codeblock:** no magic dumps. Every block has line-by-line tables + a `change X → Y` experiment.
- **Graphs everything:** every lesson ships `.excalidraw` source + exported `.svg` so you can redraw it yourself.
- **Runs on Linux-first:** WSL2 / native Linux canonical, Docker fallback. QEMU + GDB everywhere.
- **Online:** Astro + Starlight site with dependency graphs + interactive visualizers.

## Quickstart (Linux-first)

**Option A — native Linux / WSL2 (canonical):**

```bash
sudo apt update && sudo apt install -y qemu-system-x86 gdb gcc-multilib make curl
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
git clone <this-repo> osfs && cd osfs
make -C phases/P-01-c-from-zero/03-c-pointers/code run
```

What this does: installs the emulator + debugger + C toolchain + Rust, clones the repo, then builds and runs the pointers simulator. See `phases/00-tooling-linux-qemu/` for the full checklist.

| Lines | Means |
|---|---|
| `apt install ...` | `qemu-system-x86` = PC emulator, `gdb` = debugger, `gcc-multilib` = 32/64-bit C, `make` = build runner |
| `rustup` | installs `cargo` + `rustc`, needed for P-03 and later allocators |
| `make ... run` | compiles the host-side simulator (no QEMU needed yet) and runs it |

**Option B — Docker (fallback, same toolchain):**

```bash
docker build -t osfs -f docker/Dockerfile .
docker run --rm -it -v "$PWD:/osfs" osfs bash
```

What this does: builds an image with the exact versions from the Dockerfile, then drops you in a shell with the repo mounted. All `make`/`cargo` commands work identically inside.

## Repo layout

```text
phases/
  P-00-mental-model/
  P-01-c-from-zero/
  P-02-asm-from-zero/      # GAS AT&T syntax (Linux default)
  P-03-rust-from-zero/
  00-tooling-linux-qemu/
  01-what-is-os/ … 10-boot-to-shell/
    <MM>-slug/
      docs/en.md           # lesson narrative
      code/                # Makefile, *.c/*.s/*.rs, tests/
      figures/*.excalidraw + *.svg
      quiz.json            # 1 pre + 3 check + 2 post
      outputs/             # runbook / cheatsheet / template
site-astro/                # Astro + Starlight online site
scripts/                   # audit_lessons.py, check_figures.py, build_graph.py
docker/Dockerfile
```

## How a lesson works

1. **Concept in 60s** — one Excalidraw diagram, plain English.
2. **Simulate It** — host-side C/Rust program, runs without QEMU.
3. **Build It** — bare-metal / QEMU piece.
4. **Use It** — observe the same idea on real Linux (`strace`, `/proc`, `ftrace`).
5. **Ship It** — reusable artifact in `outputs/` (runbook, template, debug script).
6. **Verify** — quiz + `change X → Y` experiment + expected output as evidence.

Every codeblock is explained line-by-line. See `LESSON_TEMPLATE.md`.

## Assembly syntax: why AT&T (GAS)

Most of Linux — kernel source, `objdump`, `gdb` default disassembly — uses **AT&T syntax** (GAS). So we use AT&T as canonical:

```asm
movl $1, %eax   # $ = immediate, % = register, src → dst
```

What this does: puts the number `1` into register `%eax`. The `l` suffix means 32-bit.

| Part | Means |
|---|---|
| `movl` | move, 32-bit (`b`=8, `w`=16, `l`=32, `q`=64) |
| `$1` | immediate value 1 (`$` = literal, no `$` = memory address) |
| `%eax` | register eax (`%` = register in AT&T) |
| order | AT&T is `src, dst` — opposite of Intel. `mov $1, %eax` = Intel `mov eax, 1` |

We include an Intel↔AT&T cheat table in every ASM lesson (`P-02`), plus `.intel_syntax noprefix` notes where GDB can switch. You only need ~15 instructions for the whole OS track.

## Status

See `ROADMAP.md`. Primer + Tooling first, then OS core in order. Site skeleton in `site-astro/`.

## Contributing

Read `AGENTS.md` before any PR. One commit per lesson, every codeblock explained, every SVG has an `.excalidraw` source, Linux-first (must pass in Docker).

# WSL2 + Docker Setup — One Toolchain, Two Doors

> Same compiler everywhere. Native when you can, container when you must.

**Type:** Build
**Languages:** Bash
**Prerequisites:** None
**College ref:** OSTEP Ch.1 Dialogue (what the OS hides), MIT 6.1810 Lab tools (QEMU/GDB), xv6 file Makefile (toolchain preview)
**Time:** ~60 minutes

## Learning Objectives
- Trace where your compiler, emulator, and debugger live (WSL vs Docker)
- Implement a one-command toolchain check that exits 0 only when all tools pass
- Explain why Linux-first matters (fork, signals, /proc only exist there)
- Connect this setup to every later lesson: same `make run` everywhere

## Concept in 60s

![toolchain map](../figures/toolchain-map.svg)

<!-- source: ../figures/toolchain-map.excalidraw — open in excalidraw.com to redraw -->

You have two doors into the same Linux room. Door 1: **WSL2 / native Linux** — fastest, real `fork()`, real `/proc`. Door 2: **Docker** — same Ubuntu 24.04 image for Mac/Windows-without-WSL or CI. Both doors give you `gcc + qemu-system-x86_64 + gdb + rustc + make`. Every later lesson runs `make run` identically inside either door. Pick Door 1 if you can; keep Door 2 as fallback. See [syscall](../../glossary/terms.md#syscall).

## Simulate It (host, no QEMU)

Full program: `code/verify.py`. No root, no emulator — just version checks.

```python
import shutil, subprocess, sys

TOOLS = ["gcc", "make", "gdb", "qemu-system-x86_64", "rustc"]

missing = [t for t in TOOLS if shutil.which(t) is None]
print("missing:", missing if missing else "none")
sys.exit(1 if missing else 0)
```

What this does: looks each required binary up on `PATH` and exits non-zero if anything is absent, so `make` fails loudly instead of halfway through a kernel build.

| Lines | Code | Why it exists |
|---|---|---|
| 1 | `import ...` | stdlib only — must run on a bare WSL install with zero pip packages |
| 3 | `TOOLS = [...]` | canonical set: C compiler, build runner, debugger, emulator, Rust — the exact set `docker/Dockerfile` installs |
| 5 | `shutil.which` | `which` on PATH; avoids running anything, just presence |
| 6–7 | print + exit | human-readable + machine-readable: CI gates on exit code |

Change X → Y: temporarily rename one entry, e.g. `"gdb"` → `"gdb-NOT-REAL"`. Verify: `python3 code/verify.py; echo $?` → prints `missing: ['gdb-NOT-REAL']`, exit `1` (proves the check actually fails instead of always passing).

Full checker in repo also prints versions — same idea, more lines:

```bash
python3 code/verify.py && echo "toolchain OK"
```

What this does: runs the full `code/verify.py` (presence + `--version` probes) and confirms with a single line you can paste as evidence.

| Lines | Code | Why it exists |
|---|---|---|
| `python3 code/verify.py` | run checker | exit 0 = all tools found, non-zero = printed what's missing |
| `&& echo ...` | gate the confirmation | `&&` means "only print OK if the checker passed" — no false confidence |

Change X → Y: run `python3 code/verify.py --strict` (also requires `qemu-system-i386`). Verify: on minimal installs it fails naming the missing binary — install it or drop `--strict`.

## Build It (pick your door)

Door 1 — WSL2 / native Linux (canonical):

```bash
sudo apt update && sudo apt install -y qemu-system-x86 gdb gcc-multilib make python3
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
python3 code/verify.py && echo "door 1 OK"
```

What this does: installs the exact Ubuntu set from `docker/Dockerfile`, installs Rust, then proves it with the checker.

| Lines | Code | Why it exists |
|---|---|---|
| `apt install ...` | emulator + debugger + C + build + python | `qemu-system-x86` = both x86_64 + i386 emulators; `gcc-multilib` = 32-bit kernels later; `gdb` = kernel debugging |
| `rustup ... -y` | Rust toolchain | `cargo`/`rustc` for P-03 + allocators; `-y` = non-interactive for scripts |
| `verify.py && echo` | proof | same gate as Simulate It — save this terminal output as evidence |

Change X → Y: skip `gcc-multilib` and later try building a 32-bit boot sector. Verify: link fails with `cannot find -m32` — that error means "go back and install multilib".

Door 2 — Docker (fallback / CI-identical):

```bash
docker build -t osfs -f docker/Dockerfile .
docker run --rm -it -v "$PWD:/osfs" -w /osfs osfs python3 code/verify.py
```

What this does: builds the pinned toolchain image, then runs the same checker *inside* the container with your repo mounted.

| Lines | Code | Why it exists |
|---|---|---|
| `docker build ...` | frozen toolchain | `docker/Dockerfile` pins Ubuntu 24.04 + versions; CI runs this exact image |
| `-v "$PWD:/osfs"` | mount repo in | edits on host appear instantly in container; no copy step |
| `-w /osfs ... verify.py` | same proof, inside | if this prints versions, Door 2 is ready; use `docker run ... bash` for interactive work |

Change X → Y: drop `-v` and run `ls /osfs`. Verify: empty — proves the mount is what gives the container your files (common beginner confusion).

## Use It (Linux)

Confirm you're actually on Linux and see what the OS gives you:

```bash
uname -a
cat /proc/version
which gcc gdb qemu-system-x86_64 rustc make
```

What this does: prints kernel + distro, then resolves every tool path so "command not found" becomes impossible to misdiagnose.

| Lines | Code | Why it exists |
|---|---|---|
| `uname -a` | kernel identity | `Linux ... x86_64 GNU/Linux` = Door 1; `...-microsoft-standard-WSL2` = WSL specifically |
| `cat /proc/version` | distro + gcc that built the kernel | proves `/proc` exists — a Linux-only superpower later lessons use constantly |
| `which ...` | PATH resolution | shows *which* copy runs (WSL vs Docker vs stray Windows `.exe`) |

Change X → Y: run `which -a gcc` on WSL with a Windows gcc also installed. Verify: two paths — the first wins; if it's `/mnt/c/...`, your PATH prefers Windows and later `make` will fail. Fix by putting `/usr/bin` first or working inside Docker.

## Ship It

Artifact: `outputs/toolchain-card.md` — fill-in-the-blanks card (OS, kernel, gcc/qemgdb/rustc versions, door used). Paste it into any bug report or PR so others reproduce your env in one glance.

Reuse in 2 lines: re-run `python3 code/verify.py` before each phase; if it fails, re-read the card and reinstall only the missing piece.

## Exercises

1. Easy — run `python3 code/verify.py`, save full output + exit code as your first evidence file.
2. Medium — inside Docker, run `cat /proc/version` and compare to host. Explain why they match on Linux but differ on Mac.
3. Hard — break your PATH on purpose (`PATH=/usr/bin:/bin python3 code/verify.py`) and read which tools vanish. Restore with `export PATH=...` or a new shell.

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| syscall | user→kernel trap; Linux-only behavior we depend on | ../../glossary/terms.md#syscall |
| freestanding | kernel C without libc; why we need multilib + linker later | ../../glossary/terms.md#freestanding |

## Further Reading

- OSTEP Ch.1 — what the OS virtualizes; why we test on real Linux, not a mock.
- `docker/Dockerfile` in this repo — the pinned list; read it as the spec, not folklore.

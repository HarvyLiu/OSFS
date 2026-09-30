# Roadmap

Status tracker. Glyphs feed the site (`site-astro` parses them); do not change their shape.

**Legend:** ✅ Complete · 🚧 In Progress · ⬚ Planned

Total target: ~110–130 hours (primer ~35h + tooling ~5h + OS core ~70–90h), at your own pace.

## Primer — zero to OS-ready (~35h)

### P-00 Mental model — ⬚ (~4h)

| # | Lesson | Status | Est. |
|---|--------|--------|------|
| 01 | Binary, bits, hex — reading memory | ✅ | ~45 min |
| 02 | How programs run — compile, link, load | ✅ | ~60 min |
| 03 | Memory map — stack vs heap vs static | ✅ | ~60 min |
| 04 | Debugging mindset — GDB, asserts, logs | ✅ | ~60 min |

### P-01 C from zero — 🚧 (~12h)

| # | Lesson | Status | Est. |
|---|--------|--------|------|
| 01 | Hello C, types, control flow | ✅ | ~60 min |
| 02 | Functions, arrays, strings | ✅ | ~60 min |
| 03 | Pointers — addresses, deref, arithmetic | ✅ | ~90 min |
| 04 | malloc/free, heap ownership | ✅ | ~75 min |
| 05 | structs, enums, typedef — PCB preview | ✅ | ~75 min |
| 06 | Headers, source split, Makefile | ✅ | ~75 min |
| 07 | Preprocessor, freestanding vs hosted | ✅ | ~60 min |

### P-02 ASM from zero (GAS AT&T) — ⬚ (~8h)

| # | Lesson | Status | Est. |
|---|--------|--------|------|
| 01 | Registers, mov, add/sub — first trace | ✅ | ~60 min |
| 02 | Stack, push/pop, call/ret, frames | ✅ | ~75 min |
| 03 | Memory addressing modes, lea vs mov | ✅ | ~60 min |
| 04 | Interrupts, cli/sti, IDT preview | ✅ | ~75 min |
| 05 | C↔ASM calling, inline asm basics | ✅ | ~75 min |
| 06 | Linker scripts — where kernel lives | ✅ | ~60 min |

### P-03 Rust from zero — ⬚ (~11h)

| # | Lesson | Status | Est. |
|---|--------|--------|------|
| 01 | Cargo, ownership, borrow checker intuition | ✅ | ~75 min |
| 02 | Structs, enums, match, Option/Result | ✅ | ~75 min |
| 03 | Traits, generics, vectors/boxes | ✅ | ~75 min |
| 04 | no_std, panic handlers, unsafe boundaries | ✅ | ~90 min |
| 05 | C FFI — calling Rust from kernel C | ✅ | ~75 min |

## OS core

### 00 Tooling (Linux-first) — 🚧 (~5h)

| # | Lesson | Status | Est. |
|---|--------|--------|------|
| 01 | WSL2 + Docker setup, verify toolchain | ✅ | ~60 min |
| 02 | QEMU hello, GDB attach, Make loop | ✅ | ~75 min |

### 01 What is an OS — ⬚ (~5h)

| # | Lesson | Status | Est. |
|---|--------|--------|------|
| 01 | Abstractions — files, procs, address spaces | ✅ | ~45 min |
| 02 | Syscalls — strace your first trap | ✅ | ~75 min |

### 02 Processes — 🚧 (~8h)

| # | Lesson | Status | Est. |
|---|--------|--------|------|
| 01 | PCB — the process on paper | ✅ | ~60 min |
| 02 | fork/exec/wait — concept then code | ✅ | ~75 min |
| 03 | Context switch — stack surgery (AT&T) | ✅ | ~90 min |

### 03 Scheduling — ⬚ (~8h)

| # | Lesson | Status | Est. |
|---|--------|--------|------|
| 01 | FIFO, SJF, RR — simulators | ✅ | ~75 min |
| 02 | MLFQ — rules + starvation | ✅ | ~90 min |
| 03 | CFS intuition — vruntime | ✅ | ~60 min |

### 04 Concurrency — ⬚ (~10h)

| # | Lesson | Status | Est. |
|---|--------|--------|------|
| 01 | Threads vs procs, races | ✅ | ~60 min |
| 02 | Locks — ticket + spin | ✅ | ~90 min |
| 03 | Semaphores, condvars, deadlock | ✅ | ~90 min |

### 05 Memory I: spaces + paging — 🚧 (~8h)

| # | Lesson | Status | Est. |
|---|--------|--------|------|
| 01 | Address spaces — the private-everything lie | ✅ | ~60 min |
| 02 | Segmentation + paging walkthrough | ✅ | ~75 min |
| 03 | Multi-level page tables in code | ✅ | ~90 min |

### 06 Memory II: allocators, TLB, swap — 🚧 (~6h)

| # | Lesson | Status | Est. |
|---|--------|--------|------|
| 01 | Bump allocator (Rust) | ✅ | ~75 min |
| 02 | TLB behavior + effective access time | ✅ | ~60 min |
| 03 | Swap, demand paging, OOM | ✅ | ~75 min |

### 07 Persistence: blocks + devices — 🚧 (~6h)

| # | Lesson | Status | Est. |
|---|--------|--------|------|
| 01 | Block layer — cache, flush, crash | ✅ | ~75 min |
| 02 | RAMdisk block driver | ✅ | ~75 min |

### 08 Filesystems: files on blocks — 🚧 (~8h)

| # | Lesson | Status | Est. |
|---|--------|--------|------|
| 01 | VSFS — super, bitmap, inodes, dirs | ✅ | ~90 min |
| 02 | Journaling + crash consistency | ✅ | ~90 min |
| 03 | FFS intuition — locality + groups | ✅ | ~60 min |

### 09 Protection: rings + identity — ✅ (~5h)

| # | Lesson | Status | Est. |
|---|--------|--------|------|
| 01 | Rings + syscall gates | ✅ | ~60 min |
| 02 | Users, groups, capabilities | ✅ | ~60 min |

### 10 Boot-to-shell capstone — ✅ (~12h)

| # | Milestone | Status | Est. |
|---|-----------|--------|------|
| 01 | Boot sector — 512B print + hang | ✅ | ~90 min |
| 02 | Protected mode + C kernel entry | ✅ | ~120 min |
| 03 | Paging + scheduler + tiny FS + shell | ✅ | ~8h |

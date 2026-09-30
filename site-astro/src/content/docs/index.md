---
title: OS From Scratch
description: College OS for beginners — graphed, runnable, Linux-first.
---

College OS, rewritten so a beginner can actually follow it.

- **Concept in 60s:** one Excalidraw diagram per lesson (`.excalidraw` source committed).
- **Every codeblock explained:** line tables + `change X → Y` experiments.
- **Linux-first:** WSL2 / native Linux canonical, Docker fallback, QEMU + GDB everywhere.
- **AT&T (GAS) assembly:** what Linux itself uses. Intel shown only as a side-table.

Start with the primer, then follow the phases in order. Suggested path:

1. **Primer:** [Bits, binary, hex](lessons/p-00-mental-model--01-bits-binary-hex/) → [How programs run](lessons/p-00-mental-model--02-how-programs-run/) → [Memory map](lessons/p-00-mental-model--03-memory-map/) → [Debugging](lessons/p-00-mental-model--04-debugging/)
2. **Core:** C → ASM → Rust primers, then tooling, processes, scheduling, concurrency, memory, persistence, filesystems, protection
3. **Capstone:** [Boot sector](lessons/10-boot-to-shell--01-boot-sector/) → [Protected mode](lessons/10-boot-to-shell--02-protected-mode/) → [Shell capstone](lessons/10-boot-to-shell--03-shell-capstone/)

Full status lives in the [Roadmap](roadmap/). First-used terms link to the [Glossary](glossary/).

# Shell Capstone — Paging, Timer, Tasks, FS, Prompt

> One `kernel.c`, five subsystems: identity paging, 100 Hz timer, two stacks switching, a ROM filesystem, and a shell that proves it all over serial.

**Type:** Build
**Languages:** ASM, C
**Prerequisites:** 02-protected-mode
**College ref:** OSTEP Ch.15+7+39+45 (paging, scheduling, FS, shell — the greatest hits, one binary), MIT 6.1810 trap/lab cycle, Intel SDM Vol.3 Ch.4+6 (paging, interrupts)
**Time:** ~8 hours (guided tour: 90 minutes)

## Learning Objectives
- Identity-map 4 MiB with a page directory + table and enable `CR0.PG`
- Remap the PIC, program the PIT, and count timer ticks through an IDT gate
- Switch between two stacks with a 20-line `switch_to` (02's context switch, bare metal)
- Serve files from a ROM table and a line-editing shell over serial

## Concept in 60s

![myos map](../figures/myos-map.svg)

<!-- source: ../figures/myos-map.excalidraw — open in excalidraw.com to redraw -->

Every phase converges here. **Paging** (06): one directory + one table mapping virtual = physical for the first 4 MiB — translation on, but transparent (prove the machinery before using it for isolation). **Interrupts** (04): the PIT fires IRQ0 100×/second; the PIC (remapped past CPU faults at `0x20`) vectors it through your IDT to `timer_stub`, which counts `ticks` and raises `need_resched`. **Tasks** (02/03): `switch_to` parks one [stack](../../glossary/terms.md#stack) and rides another — the shell yields every loop, the spinner counts, `tasks` prints both `esp` values as proof. **FS** (08): a ROM table of name→content (inodes without the disk — the interface is the lesson). **Shell** (01): `readline` + `dispatch` over serial — the same read-eval loop Thompson wrote, minus sixty years of features.

Honest label: the switch is *timer-flagged, cooperatively executed* — the timer contests the CPU, the shell yields at loop top. True async preemption (switch inside the ISR) is exercise 3.

## Simulate It (host — shell logic without hardware)

The FS + dispatch layer is pure C — test it on the host by stubbing the hardware half:

```bash
python3 -c "
fs = {'hello.txt': 'hello from myos fs', 'about.txt': 'myos: one kernel.c'};
print('ls:', sorted(fs));
print('cat hello.txt:', fs.get('hello.txt', 'no such file'));
print('cat missing:', fs.get('nope', 'no such file'))"
```

What this does: models `fs_cat` + `cmd_ls` semantics (lookup-or-message — the shell contract in three lines).

| Lines | Code | Why it exists |
|---|---|---|
| `fs.get(..., default)` | contract model | `fs_cat` returns 0 on miss, `cmd_cat` prints `no such file` (NULL-as-answer again — P-00/04's rule, now a feature) |

Change X → Y: `fs.get('nope', ...)` → `fs['nope']`. Verify: `KeyError` (hostexception models the crash we *don't* ship — checked lookup vs blind index, the whole lesson in one exception).

## Build It (five subsystems, one file + 20 lines of ASM)

Full files: `code/boot.s` + `code/link.ld` (inherited from 10/02 — same floor), `code/kentry.s` (same stub), `code/irq.s` (new), `code/kernel.c` (~330 lines). Linux-only build (`-m32`, QEMU).

Paging (06 alive):

```c
for (int i = 0; i < 1024; i++) page_tab[i] = (i * 4096) | 3;
page_dir[0] = (uint32_t)(uintptr_t)page_tab | 3;
```

What this does: maps virtual `0–4 MiB` to physical `0–4 MiB` (identity — every [address](../../glossary/terms.md#address) still works, now *translated*).

| Lines | Code | Why it exists |
|---|---|---|
| `\| 3` | present+writable | bits 0+1 (absent pages fault — the other 1023 directory entries are 0 on purpose: touch high memory, meet your first page fault) |
| `aligned(4096)` | hardware law | CR3 and table links demand 4 KiB alignment (low 12 bits are flags — misaligned tables corrupt silently) |
| `jmp 1f` after PG | prefetch flush | CPU may hold decoded pre-PG state (one jump, zero subtle stale-TLB bugs — paranoia costs 2 bytes) |

Interrupts (04 with hardware):

```c
outb(0x21, 0x20); ...   // PIC: IRQ0 -> vector 0x20 (past CPU exceptions 0-31)
outb(0x40, ...);        // PIT: 1193180/100 = 100 Hz square wave
__asm__ volatile("sti");
```

What this does: remaps the interrupt controllers so hardware IRQs don't collide with CPU faults, starts the metronome, enables interrupts.

| Lines | Code | Why it exists |
|---|---|---|
| remap `0x20/0x28` | no collision | default IRQ0 = vector 8 = double-fault's neighbor (IBM's 1981 overlap — every OS remaps, now you know why) |
| mask `0xFE/0xFF` | timer only | all IRQs masked except IRQ0 (keyboard is a future driver — masked hardware is polite hardware) |
| `0x8E` gate | ring-0 ISR | present + 32-bit interrupt gate (user mode would need `0xEE` — 09's rings, foreshadowed) |

`irq.s` — the stub and the switch:

```asm
timer_stub:
    pushal
    call timer_handler
    popal
    iret
```

What this does: saves all registers, runs C, restores, returns *from interrupt* (resumes the exact pre-tick instruction — preemption's whole trick in 4 lines).

| Lines | Code | Why it exists |
|---|---|---|
| `iret` not `ret` | full resume | pops eip+cs+eflags (a `ret` would resume with interrupts still masked — the machine slowly going deaf, one tick at a time) |

```asm
switch_to:  # (old_sp*, new_sp): park esp, ride other, ret into it
```

What this does: 02's context switch reduced to essence — callee-saved regs pushed, `esp` stored/loaded, `ret` pops the *new* stack's return address (first ride lands on the prepared `spinner` entry — garbage data regs, documented in-source).

Tasks + shell:

```c
for (;;) { schedule(); puts2("myos> "); readline(); dispatch(); }
```

What this does: the shell yields every loop (`switch_to` round-trips the spinner), then reads a line (serial, with backspace) and dispatches (`help ls cat echo uptime tasks`).

| Lines | Code | Why it exists |
|---|---|---|
| `schedule()` first | yield before prompt | spinner runs even while the user thinks (watch `spins` grow between two `tasks` calls — concurrency you can *see*) |
| `tasks` prints esps | proof | two `esp` values 4 KiB apart = two stacks actually switched (claims need evidence — the shell exhibits its own scheduler) |

Build + boot (Linux):

```bash
make
make qemu     # then type: help, ls, cat hello.txt, uptime, tasks
```

What this does: builds the kernel (several sectors now — watch `LOAD_SECTORS` grow past 10/02's count), boots, drops you at `myos>`.

## Use It (Linux — the demo script)

At the serial prompt (`-nographic` terminal; `Ctrl-A X` to quit QEMU):

```text
myos> help
cmds: help ls cat <f> echo <...> uptime tasks
myos> cat hello.txt
hello from myos fs
myos> uptime
3s (ticks=312)
myos> tasks
spins=1841 main_esp=0xXXXX spin_esp=0xXXXX live_esp=0xXXXX
myos> tasks
spins=2907 main_esp=0xXXXX spin_esp=0xXXXX live_esp=0xXXXX
```

What this does: exercises FS (cat), timer (uptime ticks climbing), scheduler (spins growing between calls — the other stack *ran* while you typed).

| Lines | Code | Why it exists |
|---|---|---|
| two `tasks` calls | motion proof | spins differ (something incremented without you asking — that something is task two; `live_esp` sits just below `main_esp` — same stack, call depth apart) |
| `uptime` ticks | 100 Hz proof | ~100/second (PIT divisor math, confirmed by counting — hardware keeps promises) |

Change X → Y: `cat about.txt` (the second ROM file). Verify: one-line manifesto (FS with two files is still an FS — `ls` said so, `cat` proves it).

Structure without QEMU: `python3 tests/test_shell.py -v` (6 checks: alignment, magic, banner, paging message, FS bytes, `cli`-first — layout correctness, same doctrine as 10/01–02).

## Ship It

Artifact: `outputs/myos-tour.md` — the five-subsystem demo script above plus the `tasks`-twice motion proof. This is the course's final artifact: every lesson from P-00 (stacks, argv-like parsing, debugging) through 09 (rings foreshadowed in `0x8E`) has a line of code in this kernel.

## Exercises

1. Easy — add a third ROM file, rebuild, `ls` + `cat` it (FS write path without a disk driver — the table is the disk).
2. Medium — add `echo` redirection (`echo hi > note.txt` into a writable RAM slot): first *mutable* file (ROM→RAM is the whole history of filesystems, compressed).
3. Hard — switch inside `timer_handler` when `need_resched` (true preemption: save `main_sp` in the ISR, load `spin_sp`, and solve the re-entrancy — this is OSTEP Ch.7's exam, bare metal).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| address | virtual = physical today (identity), but translated — isolation is one table away | ../../glossary/terms.md#address |
| stack | two of them, switched live (`esp` printed as evidence) | ../../glossary/terms.md#stack |
| syscall | shell commands are syscalls without the trap (dispatch table = syscall table, minus rings) | ../../glossary/terms.md#syscall |
| register | `pushal/iret` preserve the world across ticks (preemption = manners) | ../../glossary/terms.md#register |

## Further Reading

- OSTEP Ch.7 + Ch.15 + Ch.39 (scheduling, paging, FS — the three pillars, one kernel).
- MIT 6.1810 trap + scheduler labs (their preemption, our `need_resched` — compare honesty labels).
- Intel SDM Vol.3 Ch.4 + Ch.6 (paging structures, interrupt gates — authoritative).

# QEMU Hello + GDB Attach — Your First Bare-Metal Print

> If you can print one line with no OS under you, you can build an OS.

**Type:** Build
**Languages:** C, ASM
**Prerequisites:** 01-wsl-docker-setup
**College ref:** OSTEP Ch.1 (virtualization preview), MIT 6.1810 Lab 1 (boot + GDB), xv6 file kernel/entry.S + kernel/uart.c (serial idea)
**Time:** ~75 minutes

## Learning Objectives
- Trace the boot path: QEMU loads ELF → `_start` (ASM) → `kernel_main` (C)
- Implement serial print via port `0x3F8` using `outb` in AT&T inline ASM
- Explain `-ffreestanding`, linker script, multiboot magic, `cli/hlt`
- Connect GDB `target remote :1234` to a halted guest and break in `kernel_main`

## Concept in 60s

![qemu gdb map](../figures/qemu-gdb-map.svg)

<!-- source: ../figures/qemu-gdb-map.excalidraw — open in excalidraw.com to redraw -->

No `printf` here. QEMU emulates a PC; your `kernel.elf` *is* the OS. `_start` sets a [stack](../../glossary/terms.md#stack), calls C, C pokes bytes at serial port `0x3F8`, QEMU forwards them to your terminal (`-nographic`). GDB attaches over TCP `:1234` (`-S -s`) to freeze and inspect — same GDB you'll use for every later crash. See [freestanding](../../glossary/terms.md#freestanding), [register](../../glossary/terms.md#register).

## Simulate It (host — understand serial math first, no QEMU)

Serial is just "byte out to a port". On host we fake the port with a buffer so the logic is testable:

```c
#include <stdio.h>
#include <string.h>

static char fake_serial[64];
static int pos = 0;
static void fake_outb(char c) { if (pos < 63) fake_serial[pos++] = c; }

int main(void) {
    const char *msg = "OSFS\n";
    for (int i = 0; msg[i]; i++) fake_outb(msg[i]);
    fake_serial[pos] = 0;
    printf("would-send-to-0x3F8: %s", fake_serial);
    return strcmp(fake_serial, "OSFS\n") != 0;
}
```

What this does: loops over a string and "sends" each byte through a fake port function, then proves the buffer holds exactly what the guest would emit.

| Lines | Code | Why it exists |
|---|---|---|
| 4–5 | `fake_serial` + `fake_outb` | stand-in for hardware port `0x3F8`; same call shape as real `outb`, but testable on host |
| 8 | `msg = "OSFS\n"` | the exact bytes QEMU must show; `\n` flushes the terminal line |
| 9 | `for ... msg[i]` | C string = bytes ending in `0`; this loop is what `kernel_main` repeats with real `outb` |
| 11–12 | print + return | human proof + machine proof: non-zero exit fails `make test` |

Change X → Y: change `"OSFS\n"` to `"hi\n"`. Verify: `cc sim.c -o /tmp/s && /tmp/s` → prints `would-send-to-0x3F8: hi` (proves the loop drives output, not a hardcoded string).

## Build It (QEMU bare-metal)

Files in `code/`: `boot.s` (AT&T, multiboot + entry), `kernel.c` (freestanding serial), `linker.ld` (load at 1M), `Makefile`.

Entry — AT&T GAS canonical:

```asm
.set MAGIC, 0x1BADB002
.set FLAGS, 0x0
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .text
.globl _start
_start:
    movl $stack_top, %esp
    call kernel_main
    cli
hang:
    hlt
    jmp hang

.section .bss
.space 16384
stack_top:
```

What this does: tells QEMU "I'm a bootable kernel" (multiboot magic), sets a [stack](../../glossary/terms.md#stack), calls C, then halts forever if C returns.

| Lines | Code | Why it exists |
|---|---|---|
| 1–3 | `MAGIC/FLAGS/CHECKSUM` | multiboot1 contract: QEMU scans for `0x1BADB002`; checksum must sum to zero or it refuses to boot |
| 6–9 | `.long` trio | the actual 12-byte header the scanner finds; miss one and you get `not a bootable kernel` |
| 14–15 | `.globl _start` | export entry so `linker.ld ENTRY(_start)` can find it |
| 17 | `movl $stack_top, %esp` | AT&T `src→dst`: load stack address into `%esp`; C needs a [stack](../../glossary/terms.md#stack) before one `call` |
| 18 | `call kernel_main` | jump to C, push return address (never actually returns) |
| 19–22 | `cli/hlt/jmp` | `cli` = ignore interrupts, `hlt` = sleep CPU, `jmp hang` = if woken, sleep again — safe park |

Change X → Y: change `$stack_top` to `$0`. Verify: `make qemu` triple-faults or hangs silently (proves C crashed with no stack — the exact bug this line prevents).

| AT&T (we write) | Intel (you may read elsewhere) | Note |
|---|---|---|
| `movl $stack_top, %esp` | `mov esp, OFFSET stack_top` | `$`=immediate, `%`=register, `l`=32-bit |
| `outb %al, %dx` pattern via C inline below | `out dx, al` | AT&T is src,dst; Intel is dst,src |

Kernel C — freestanding, no libc:

```c
// kernel.c — freestanding: no printf, no libc. Writes "OSFS" to COM1.
#include <stdint.h>

#define COM1 0x3F8

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static void serial_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

static void serial_putc(char c) {
    while (( ({ uint8_t s; __asm__ volatile ("inb %1, %0" : "=a"(s) : "Nd"((uint16_t)(COM1 + 5))); s; }) & 0x20) == 0) {}
    outb(COM1, (uint8_t)c);
}

void kernel_main(void) {
    serial_init();
    const char *m = "OSFS hello on serial\n";
    for (int i = 0; m[i]; i++) serial_putc(m[i]);
    for (;;) { __asm__ volatile ("hlt"); }
}
```

What this does: initializes COM1 to 38400 8N1, busy-waits for "transmitter empty", sends each byte, then halts — the smallest observable kernel.

| Lines | Code | Why it exists |
|---|---|---|
| 3 | `#define COM1 0x3F8` | PC-standard first serial port; QEMU maps it to stdio under `-nographic` |
| 5–7 | `outb` inline ASM | `outb %al, %dx`: `"a"` forces value into `%al`, `"Nd"` allows imm8/`%dx` port; `volatile` = don't optimize the hardware poke away |
| 9–17 | `serial_init` | divisor + line + FIFO + modem dance; copy-paste standard, required once or bytes garble |
| 20–22 | `serial_putc` + `inb` wait | line-status bit `0x20` = THR empty; spin until 1, then emit; polling (no interrupts yet — Phase 04 topic) |
| 25–29 | `kernel_main` | freestanding entry called from `_start`; infinite `hlt` loop parks CPU after print |

Change X → Y: change `"OSFS hello on serial\n"` to `"OSFS 123\n"`. Verify: `make qemu` shows your new line (proves bytes came from *your* C, not QEMU branding).

Linker — where the kernel lives:

```ld
ENTRY(_start)
SECTIONS {
    . = 1M;
    .text : { *(.multiboot) *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : { *(.bss) }
}
```

What this does: pins the ELF so QEMU loads code at 1 MiB (classic low-memory spot above BIOS area) with multiboot header first.

| Lines | Code | Why it exists |
|---|---|---|
| `ENTRY(_start)` | entry symbol | must match `.globl _start` in `boot.s` or link fails |
| `. = 1M;` | load address | 0x100000 — safely above real-mode IVT/BDA, below everything else |
| `*(.multiboot)` first | header placement | QEMU scans first 8 KiB; if `.text` came first the magic hides and boot fails |

Change X → Y: change `1M` to `0x100000` (same value, explicit hex). Verify: `make` still boots — proves the two spellings are identical.

Run + debug:

```bash
make qemu          # timeout 5 qemu-system-x86_64 -nographic -kernel build/kernel.elf
make debug         # terminal 1: qemu -S -s ... | terminal 2: gdb -ex 'target remote :1234' ...
```

What this does: `qemu` boots and forwards serial to your terminal; `debug` freezes at first instruction so GDB owns time.

| Lines | Code | Why it exists |
|---|---|---|
| `make qemu` | boot + watch serial | `timeout 5` = never hang CI; expect `OSFS hello on serial` then timeout kill (exit 124 = success-with-timeout) |
| `make debug` | `-S -s` + GDB | `-S` = freeze, `-s` = `:1234`; `break kernel_main` + `c` lands you on your C with [registers](../../glossary/terms.md#register) inspectable |

Change X → Y: in GDB run `info registers esp` at `_start` vs inside `kernel_main`. Verify: same high value near `stack_top` — proves the ASM stack setup survived into C.

## Use It (Linux)

Confirm the host side of the same ideas:

```bash
qemu-system-x86_64 --version | head -1
gdb --version | head -1
file build/kernel.elf | cut -c1-120
```

What this does: records emulator/debugger versions plus proves your ELF is 32-bit Multiboot (not a host binary).

| Lines | Code | Why it exists |
|---|---|---|
| `qemu ... --version` | emulator pin | paste into bug reports; boot bugs are version-sensitive |
| `gdb --version` | debugger pin | `target remote` syntax is stable, but record anyway |
| `file build/kernel.elf` | ELF check | expect `ELF 32-bit LSB ...`; if it says `64-bit` or `pie`, your `-m32` flag dropped and QEMU will reject it |

Change X → Y: run `nm build/kernel.elf | grep _start`. Verify: address near `0x100000` — matches `linker.ld`, closing the loop from script → symbol → boot.

## Ship It

Artifacts in `outputs/`: `qemu-cheatsheet.md` (`run`/`debug`/`quit` keys: `Ctrl-A X` kills `-nographic`), `gdbinit-osfs` (`target remote`, `break kernel_main`, `layout asm`). Copy `gdbinit-osfs` to any later phase as starter config.

## Exercises

1. Easy — change the serial string, rebuild, show new QEMU output + `timeout` exit code.
2. Medium — in GDB, `break serial_putc`, `c`, `info registers eax edx` on hit. Explain which holds char vs port (AT&T order!).
3. Hard — remove `serial_init()` call, rebuild, describe the garbled/absent output and why baud setup matters (preview of device init).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| register | CPU-owned slot (`%esp`, `%eax`); ASM lives here | ../../glossary/terms.md#register |
| stack | call frames + locals; you built one in `boot.s` | ../../glossary/terms.md#stack |
| freestanding | no libc; you are the runtime | ../../glossary/terms.md#freestanding |

## Further Reading

- MIT 6.1810 Lab 1 — boot + GDB flow this mirrors (their RISC-V, ours x86).
- OSDev Serial + Bare Bones — init sequence + multiboot reference (practical companion).
- `man 1 qemu-system-x86_64` — `-nographic`, `-kernel`, `-S -s` definitions.

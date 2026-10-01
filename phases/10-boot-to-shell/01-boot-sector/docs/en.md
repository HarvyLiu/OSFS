# Boot Sector — 512 Bytes That Own the Machine

> BIOS loads you at 0x7C00 and jumps. No OS, no libc, no safety net — just you, 510 bytes, and a magic number.

**Type:** Build
**Languages:** ASM
**Prerequisites:** 02-qemu-gdb-hello
**College ref:** OSTEP Ch.1 (bootstrapping preview), MIT 6.1810 Lab 1 (bootloader), OSDev Bare Bones + BIOS (the rituals)
**Time:** ~90 minutes

## Learning Objectives
- Trace BIOS → 0x7C00 → print → hang using a memory map of the first KiBs
- Implement a 512-byte sector (message + VGA + serial + `0xAA55`) in 16-bit AT&T ASM
- Explain `.code16`, segment zeroing, direct VGA cells (and why BIOS teletype is banned here), and why the signature sits at bytes 510–511
- Connect this milestone to Tooling 02 (same serial dance, younger CPU mode) and to 10/02 (loading what's next)

## Concept in 60s

![boot map](../figures/boot-map.svg)

<!-- source: ../figures/boot-map.excalidraw — open in excalidraw.com to redraw -->

Power on: BIOS POSTs, finds a disk with bytes 510–511 = `0xAA55` (the "I'm bootable" handshake), loads sector 0 to physical `0x7C00`, jumps there in 16-bit real mode (1 MiB addressable, segments × 16). Your code: zero `%ds` (so addresses mean what you think), park a [stack](../../../../glossary/terms.md#stack) below `0x7C00` (it grows *down* into free low RAM), print via direct VGA cells at `0xB8000` (for humans) + COM1 serial (for CI — Tooling 02's dance, real-mode edition), `cli`/`hlt` forever. One wire, one writer: BIOS teletype (`int $0x10`) is banned on the screen path because firmware with a serial console (SeaBIOS sercon) echoes it to the wire — our CI banner once arrived as `OOSSFFSS  bboooott!`. 510 bytes code+message, 2 bytes magic. That's a bootloader's whole childhood.

## Simulate It (host — the arithmetic of layout, no QEMU)

No emulator needed to prove the *shape*: sizes, signature offset, message bytes.

```bash
echo "512-byte budget: code+msg <= 510, magic = 2"
python3 -c "print('signature offset:', 510, 'value:', hex(0xAA55))"
python3 -c "print('load addr:', hex(0x7C00), 'end:', hex(0x7C00+512))"
```

What this does: states the three numbers every byte below obeys — budget, handshake position/value, load window.

| Lines | Code | Why it exists |
|---|---|---|
| budget line | the law | `.fill` errors if code overflows (assembler enforces — negative fill count fails the build, not the boot) |
| signature line | handshake | BIOS checks *only* these bytes (everything else is yours to botch freely) |
| load line | placement | `0x7C00–0x7DFF` (31 KiB mark — low RAM's famous squatter; `. = 0x7C00` thinking, binary edition) |

Change X → Y: `hex(0x7C00+512)` → `hex(0x7C00+510)`. Verify: `0x7dfe` (magic's address — read it off before seeing `.word` below).

## Build It (16-bit AT&T — Linux-only build, like Tooling 02's `-m32`)

Full file: `code/boot.s`. Assemble with GNU `as`, link flat with `ld --oformat binary`.

```asm
# boot.s -- 512-byte boot sector: VGA + serial "OSFS boot!", then hang.
# Lesson docs/en.md. Linux-only build (16-bit real mode).
.code16
.globl _start
_start:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %ss
    movw $0x7C00, %sp
    sti
    call serial_init
    movw $0xB800, %ax
    movw %ax, %es         # ES = VGA text cells (direct: no firmware in the path)
    xorw %di, %di         # cell cursor in bytes (+2 per char)
    movb $0x07, %ah       # white-on-black (set once: serial_putc preserves %ax)
    movw $msg, %si
putc:
    lodsb
    testb %al, %al
    jz hang
    movb %al, %es:(%di)   # char cell
    movb %ah, %es:1(%di)  # attribute cell
    addw $2, %di
    call serial_putc      # %al still the char (stores don't clobber)
    jmp putc
hang:
    cli
    hlt
    jmp hang
```

What this does: enters with interrupts off, zeroes segments, parks a stack, inits serial, prints the message twice-over (VGA cells + wire), sleeps forever.

| Lines | Code | Why it exists |
|---|---|---|
| `.code16` | 16-bit mode | assembler emits real-mode encodings (default is your host's 64-bit — wrong bytes, silent death) |
| `cli` first | fence | no stack yet — an interrupt now would push into garbage (P-02/04's fence-first law, boot edition) |
| `xorw %ax,%ax` | zero fast | `xor` self = shortest zeroing (idiom compilers emit — 2 bytes, no immediate) |
| `movw %ax,%ds/%ss` | segments zero | addresses = segment×16 + offset: DS=0 makes `$msg` (0x7Cxx) land right (else everything points 16× off) |
| `movw $0x7C00,%sp` | stack below load | grows down from load base (0x7BFF↓ — free low RAM; upward would eat our own code) |
| `sti` | enable after | stack exists now — safe to take interrupts (order mirrors `cli`-first: setup, *then* expose) |
| `lodsb/testb/jz` | string walk | load byte at DS:SI++, stop at zero (`asciz` terminator — P-01/02's zero rule, bare metal) |
| `ES=0xB800, %es:(%di)` | video segment | physical `0xB8000` = text buffer (`0xB800`×16 — segments as addressing, 05 preview) |
| cells + `addw $2` | direct write | char+attribute per cell, cursor in `%di` (no BIOS call — firmware can't echo what it never sees; SeaBIOS sercon doubled our CI banner once, never again) |

```asm
serial_init:
    movw $0x3F9, %dx
    movb $0x00, %al
    outb %al, %dx
    movw $0x3FB, %dx
    movb $0x80, %al
    outb %al, %dx
    movw $0x3F8, %dx
    movb $0x03, %al
    outb %al, %dx
    movw $0x3F9, %dx
    movb $0x00, %al
    outb %al, %dx
    movw $0x3FB, %dx
    movb $0x03, %al
    outb %al, %dx
    movw $0x3FA, %dx
    movb $0xC7, %al
    outb %al, %dx
    movw $0x3FC, %dx
    movb $0x0B, %al
    outb %al, %dx
    ret

serial_putc:            # char in %al
    pushw %dx
    pushw %ax           # saves char AND %ah (caller's 0x0E survives!)
wait_ser:
    movw $0x3FD, %dx
    inb %dx, %al
    testb $0x20, %al
    jz wait_ser
    popw %ax            # char back
    movw $0x3F8, %dx
    outb %al, %dx
    popw %dx
    ret

msg:
    .asciz "OSFS boot!"

.fill 510 - (. - _start), 1, 0
.word 0xAA55
```

What this does: initializes COM1 (38400 8N1, Tooling 02's exact dance), sends each char after line-ready, terminates the sector with message + padding + magic.

| Lines | Code | Why it exists |
|---|---|---|
| init 7 writes | baud+line+FIFO+modem | same values as Tooling 02 (hardware unchanged since 1981 — the dance is eternal) |
| `pushw %ax` pair | save char across status read | status poll clobbers `%al`; stack preserves (P-02/02's frames, 2 pushes deep) |
| `$0x20` test | THR-empty bit | transmit-holding-register ready (polling — no interrupts at boot, P-02/04's world before IDT) |
| `.asciz` | zero-terminated | the `testb/jz` exit condition's data (strings end, loops end — paired by convention) |
| `.fill 510-(...)` | pad to budget | zero-fill to byte 510 (oversize code = *negative* fill = build error — budget enforced, not hoped) |
| `.word 0xAA55` | handshake | bytes 510–511 little-endian `55 AA` (BIOS reads u16 `0xAA55` — endianness made memorable) |

| AT&T 16-bit here | Note |
|---|---|
| `movw %ax, %ds` | `w` = 16-bit regs (segment regs have no `l` form — `movl %eax,%ds` won't assemble) |
| `outb %al, %dx` | same as 32-bit (ports are 16-bit-addressed either way) |
| `lodsb/testb/jz` | byte-string trio (P-01/02's `'\0'` hunt, one instruction) |

Change X → Y: change `"OSFS boot!"` to `"HI!"`, rebuild, `python3 -c` check size still 512. Verify: boots + shorter message (padding absorbs the difference — `.fill` is load-bearing arithmetic, watch it flex).

Build + boot:

```bash
make
make qemu     # timeout 5 qemu ... -nographic; expect "OSFS boot!" then timeout (124)
```

What this does: assembles 16-bit, links flat at... note: no `-Ttext` needed — `.org`? Our labels resolve from 0; `ld --oformat binary` lays from 0, and `$msg` = file offset; DS=0 + loaded at 0x7C00 means... wait: `$msg` assembles to its *file offset* (~0x30), but at runtime DS=0 and code runs at 0x7C00 → `movw $msg,%si` loads 0x30, but the message sits at 0x7C30! BUG — need origin. Fix: `ld -Ttext 0x7c00` makes `$msg` = 0x7Cxx. The Makefile below does exactly that (same as Tooling 02's linker thinking). This paragraph stays as the trap explained:

| Lines | Code | Why it exists |
|---|---|---|
| `-Ttext 0x7c00` | origin | absolute addresses assume load base (without it, `%si` points 0x7C00 low — prints BIOS-area garbage; the classic first-boot bug, documented before you meet it) |
| `-nographic` | serial to stdio | our COM1 bytes appear in terminal (VGA needs a display; serial needs only stdio — CI reads wire, humans read screen) |
| timeout 124 | expected kill | guest halts forever by design (124 = success-with-timeout, same as Tooling 02) |

## Use It (Linux)

Forensics on 512 bytes:

```bash
ls -l build/boot.bin
xxd build/boot.bin | tail -2
python3 tests/test_boot.py -v
```

What this does: sizes the image (exactly 512 — not 511, not 513), dumps the magic tail (`55 aa` at `01fe`), runs the structural suite.

| Lines | Code | Why it exists |
|---|---|---|
| `ls -l` | budget proof | 512 (linker math + fill, audited by the filesystem) |
| `xxd tail` | handshake proof | `... 55 aa` at offsets 1fe–1ff (little-endian `0xAA55` — read the dump right-to-left per x86) |

Change X → Y: `dd if=build/boot.bin of=/tmp/bs bs=1 count=3 2>/dev/null | xxd` (first 3 bytes). Verify: `fa 31 c0` = `cli; xor %ax,%ax` (your entry, disassembled by eye — `objdump -m i8086 -D` decodes the rest).

## Ship It

Artifact: `outputs/boot-checklist.md` — `.code16`, segments-zero, stack-below, origin-0x7C00, 510+magic, dual-output (VGA+serial), timeout-124. Every later boot stage (10/02+) starts from this list (loader → protected mode → C, each item re-verified).

## Exercises

1. Easy — change the message, rebuild, show new serial line + still-512 (padding flexes — `.fill` earns trust).
2. Medium — `objdump -m i8086 -D -b binary build/boot.bin | head -20` (read your boot as the CPU does — map 5 lines to source).
3. Hard — print `%sp` in hex at entry (write a `print_hex` nibble loop with `0x0E`/`outb`): prove the stackRename below 0x7C00 (bring-up observability, hand-built).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| register | `%ax/%dx/%si` run the show (16-bit slices, segment regs zeroed) | [register](../../../../glossary/terms.md#register) |
| stack | parked at 0x7C00 growing down (`call` needs it before first use) | [stack](../../../../glossary/terms.md#stack) |
| address | physical = seg×16+off (real mode has no paging — 05's world starts after) | [address](../../../../glossary/terms.md#address) |
| syscall | none yet (BIOS `int` services instead — firmware, not kernel) | [syscall](../../../../glossary/terms.md#syscall) |

## Further Reading

- OSDev Bare Bones (and its BIOS interrupts list — read `int 0x10/AH=0x0E` to learn why the screen path here avoids it).
- MIT 6.1810 boot lab (their loader, our sector — compare rituals).
- Intel SDM Vol.1 Ch.3 (real-mode addressing — seg×16+off, authoritative).

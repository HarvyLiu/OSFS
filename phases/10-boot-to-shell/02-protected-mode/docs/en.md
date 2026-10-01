# Protected Mode — One Bit, Then C Runs the Machine

> Set CR0.PE, far-jump through your own GDT, and land in a freestanding C `kmain` that owns VGA and serial.

**Type:** Build
**Languages:** ASM, C
**Prerequisites:** 01-boot-sector
**College ref:** OSTEP Ch.15 (mechanism: address spaces need hardware), MIT 6.1810 MMU/GDT lab, Intel SDM Vol.3 Ch.3 (protection)
**Time:** ~120 minutes

## Learning Objectives
- Load a multi-sector kernel with BIOS `int $0x13` (drive byte saved from `%dl`)
- Explain A20, GDT entries, `CR0.PE`, and why the far jump is mandatory
- Implement a 32-bit entry stub (segments, [stack](../../../../glossary/terms.md#stack), `call kmain`) plus freestanding C kernel
- Link with a script (`ENTRY`, origin `0x8000`) and boot it in QEMU

## Concept in 60s

![mode switch](../figures/prot-switch.svg)

<!-- source: ../figures/prot-switch.excalidraw — open in excalidraw.com to redraw -->

Real mode (10/01) is a 1 MiB playground with no protection — every program sees everything. Protected mode adds **segments with limits and privilege**: the CPU reads descriptors from your GDT (Global Descriptor Table), and `CR0.PE` (bit 0) flips the decoding from 16-bit real to 32-bit protected. The pipeline: BIOS disk read → kernel sectors at `0x8000` → A20 gate (unlock memory past 1 MiB — a PC-AT fossil, still wired) → `lgdt` → set PE → **far jump** (reloads the hidden segment cache — without it the CPU keeps decoding 16-bit) → 32-bit stub → C. After the jump, BIOS is unreachable (your segments, your rules) — which is why `kmain` writes VGA memory directly instead of calling `int $0x10`.

## Simulate It (host — the GDT on paper, no QEMU)

Decode the code descriptor before trusting it:

```bash
python3 -c "d=0x00CF9A000000FFFF; print('limit_lo=%04x base=%02x%02x%04x access=%02x gran=%02x' % (d&0xFFFF,(d>>32)&0xFF,(d>>24)&0xFF,(d>>16)&0xFFFF,(d>>40)&0xFF,(d>>48)&0xFF))"
python3 -c "print('kernel sectors for 2048B:', (2048+511)//512)"
```

What this does: splits the 8-byte descriptor into its fields, and rehearses the Makefile's sector-count math.

| Lines | Code | Why it exists |
|---|---|---|
| decode line | field check | access `0x9A` = present/code/exec-read, gran `0xCF` = 4 KiB pages × 32-bit (wrong nibble = triple fault — verify before booting) |
| sectors line | load math | `LOAD_SECTORS` must cover `kernel.bin` exactly (too few = truncated kernel, too many = BIOS error past disk end) |

| AT&T 32-bit here | Note |
|---|---|
| `movl %cr0, %eax` | `l` = 32-bit control reg move (no `w` form — control regs are 32-bit minimum) |
| `ljmp $0x08, $0x8000` | far jump: selector, offset (`src → dst` order holds — first where-in-GDT, then where-to-jump) |
| `.code32` in kentry | same mnemonic style, wider registers (P-02's 32-bit world returns — segments now mean protection) |

Change X → Y: `(2048+511)//512` → `(512+511)//512`. Verify: `1` (a tiny kernel loads one sector — the formula rounds up, never down; down would amputate).

## Build It (loader → stub → C)

Full files: `code/boot.s`, `code/kentry.s`, `code/kernel.c`, `code/link.ld`. Linux-only build (`-m32`, BIOS, QEMU).

```asm
movb %dl, drive       # save FIRST: BIOS passes boot drive in %dl
...
movw $0x800, %ax
movw %ax, %es
xorw %bx, %bx         # ES:BX = 0x8000: kernel landing zone (above 0x7C00+)
movb drive, %dl
movb $0x02, %ah       # int 0x13 read
movb $LOAD_SECTORS, %al
movb $0x00, %ch       # cylinder 0
movb $0x02, %cl       # sector 2 (we are sector 1)
movb $0x00, %dh       # head 0
int $0x13
jc disk_fail
```

What this does: fetches the kernel off disk into memory while BIOS services still exist (after the mode switch there is no disk BIOS — only your drivers, written in later lessons).

| Lines | Code | Why it exists |
|---|---|---|
| `movb %dl,drive` first | save boot drive | BIOS tells you once (`%dl` = 0x00 floppy / 0x80 disk); any `int` may clobber it — stash before anything else |
| `ES:BX=0x800:0` | landing zone | physical `0x8000` (above our 0x7C00 sector, below VGA — the conventional kernel parking spot) |
| `jc disk_fail` | carry check | BIOS reports errors in carry (unchecked = jumping into unloaded garbage — the loader's NULL check, 10/01's `jc` cousin) |
| `--defsym LOAD_SECTORS` | size-aware | Makefile counts `kernel.bin` → sectors (docs lie, arithmetic doesn't — the build measures) |

```asm
movw $0x2401, %ax
int $0x15             # A20: unwraps the 1 MiB address mirror
cli
lgdt gdt_desc
movl %cr0, %eax
orl $0x1, %eax
movl %eax, %cr0
ljmp $0x08, $0x8000
```

What this does: unlocks high memory, teaches the CPU two segments, sets the PE bit, and far-jumps into the kernel's front door.

| Lines | Code | Why it exists |
|---|---|---|
| `int $0x15/2401` | A20 gate | ancient compatibility wraps addresses at 1 MiB (enable = access your actual RAM — QEMU honors this call) |
| `lgdt` while real | load table | legal in real mode (prepares the jump — table must exist *before* PE, used *after*) |
| `orl $0x1` | PE bit | bit 0 of CR0 (the one-bit revolution — every later protection in 05/09 starts here) |
| `ljmp $0x08,$0x8000` | flush + land | selector 8 = GDT entry 1 (code); far jump reloads CS hidden cache (near jump would keep decoding 16-bit — instant garbage) |

`kentry.s` (linked first at `0x8000` — the far jump lands on its first byte):

```asm
kentry:
    movw $0x10, %ax
    movw %ax, %ds
    ... %es %fs %gs %ss
    movl $0x7000, %esp
    call kmain
```

What this does: points every data segment at GDT entry 2 (selector `0x10`), parks a 32-bit [stack](../../../../glossary/terms.md#stack), calls C.

| Lines | Code | Why it exists |
|---|---|---|
| `$0x10` all segments | flat data | base 0, limit 4G (C assumes flat memory — P-01's pointers finally tell the truth) |
| `$0x7000` stack | below kernel | grows down through free low RAM (0x8000 is occupied upward by us — same rule as 10/01, new address) |

`kernel.c` is freestanding (`-ffreestanding` — no libc, no `main`, kernel provides everything): `__asm__ outb/inb` helpers, the eternal COM1 init, `kmain` writing each char twice — VGA cell (`0x0700 | c` at `0xB8000`) for eyes, serial for CI.

Build + boot:

```bash
make
make qemu     # timeout 5 qemu ... -nographic; expect "protected! C runs." then 124
```

What this does: compiles 32-bit freestanding C, links flat at `0x8000` per `link.ld`, counts kernel sectors into the loader, concatenates `os.bin`, boots.

| Lines | Code | Why it exists |
|---|---|---|
| `-m32 -ffreestanding` | 32-bit no-host | kernel is not a Linux program (no libc startup, no syscalls — `ENTRY(kentry)` replaces `_start`) |
| `stat` sector math | measure, don't guess | `kernel.bin` grows as features land (10/03 reuses this — the Makefile scales with the kernel) |
| `cat boot+kernel` | one disk | sector 1 loader + sectors 2+ kernel (the image *is* the disk layout — `xxd` it and see) |

## Use It (Linux)

Forensics on the disk image:

```bash
ls -l build/os.bin build/kernel.bin
python3 tests/test_prot.py -v
```

What this does: sizes both (os = 512 + kernel-padded-to-sector), runs the structural suite (alignment, magic, message bytes, GDT `9A CF` fingerprint, `cli`-first entry).

| Lines | Code | Why it exists |
|---|---|---|
| suite over QEMU | structure first | boot correctness is layout correctness (bytes provable without emulation — QEMU then confirms behavior) |

Change X → Y: `objdump -m i386 -D -b binary --adjust-vma=0x8000 build/kernel.bin | head` (Linux-only). Verify: `kentry`'s segment loads then `call kmain` (read your kernel as the CPU does after the far jump — 10/01's exercise, 32-bit edition).

## Ship It

Artifact: `outputs/pmode-checklist.md` — drive saved, kernel at 0x8000, A20, GDT decoded, PE set, far jump (not near), flat segments, stack below, freestanding link, dual output, 124. 10/03 builds paging and interrupts on exactly this floor.

## Exercises

1. Easy — change the message, rebuild, confirm sector count unchanged or grown (watch the Makefile measure — add a `printf` to kmain and see sectors tick up).
2. Medium — corrupt one GDT nibble (`CF`→`4F`), rebuild, boot (triple fault / reboot loop — feel what the CPU does with a lying descriptor).
3. Hard — print `CR0` bits from kmain (inline `movl %cr0`) over serial in hex (prove PE is set from inside C — the CPU confessing to your code).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| register | `%cr0`/`%cs` run the switch (control + segment, one bit + one jump) | [register](../../../../glossary/terms.md#register) |
| address | segments now add protection (base 0/limit 4G today, real limits in 05) | [address](../../../../glossary/terms.md#address) |
| stack | re-parked at 0x7000 for 32-bit (grows down, still sacred) | [stack](../../../../glossary/terms.md#stack) |
| syscall | none (BIOS abandoned after the jump — drivers are future lessons) | [syscall](../../../../glossary/terms.md#syscall) |

## Further Reading

- Intel SDM Vol.3 Ch.3 (protected-mode entry ritual — authoritative, dense).
- MIT 6.1810 boot + GDT lab (their switch, our sector — compare).
- OSDev Protected Mode (A20 methods table — 0x2401 vs keyboard controller lore).

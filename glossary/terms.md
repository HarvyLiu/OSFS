# Glossary — canonical definitions (link here on first use)

## pointer
A variable holding a memory address. Dereference (`*p`) means "go to that address".

## address
A numbered byte slot in memory. `&x` means "address of x".

## register
Tiny CPU-owned storage (`%eax`, `%rax`, `%rsp`). Fastest memory; ASM operates here.

## stack
LIFO region for call frames + locals. Grows down on x86 (`%rsp` tracks top). See P-02-02.

## heap
Long-lived region via `malloc`/`free` (C) or allocator (Rust `no_std`). You manage lifetime.

## syscall
Controlled trap from user → kernel (`int $0x80` / `syscall` instr). See `man 2 syscalls`.

## PCB
Process Control Block — kernel's `struct` per process: pid, state, registers, page-table ptr, open files.

## page
Fixed-size (usually 4 KiB) virtual-memory chunk. Paging maps virtual pages → physical frames.

## TLB
Translation Lookaside Buffer — cache of recent page-table entries.

## inode
Filesystem's per-file metadata: type, size, block pointers. Directory maps names → inodes.

## journal
Write-ahead log for crash consistency: intend → do → checkpoint.

## freestanding
C without hosted libc (`-ffreestanding`): no `printf`/`malloc` unless you write them. Kernels are freestanding.

## panic_handler
Rust `no_std` required handler for unrecoverable errors — usually halt loop + serial print.

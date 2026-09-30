# Preprocessor card

- Paste: `#define F(a) ((a)+1)` — parens around args AND body, always.
- Fork: `#ifdef X / #else / #endif`; `-DX` flips from the command line.
- Inspect: `gcc -E` (see paste), `gcc -dM -E` (predefined), `grep -c` release audits.
- Guards: `#ifndef FILE_H` on every header, no exceptions.
- Freestanding: `-ffreestanding -nostdlib -c` compiles logic; boot.s + .ld make it run.

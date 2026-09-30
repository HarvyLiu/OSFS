# Stages card

- Flags: -E (.i paste) -S (.s asm) -c (.o object) + link (binary) + exec (process).
- Errors: paste (missing header) / compile (syntax) / link (undefined ref) / loader (no file).
- Reads: wc (paste size), head .s (dialect), nm T/U (define/use), file+ldd (loader).
- Habits: first error only; argv[0]=self; exit codes are data ($?/wait).

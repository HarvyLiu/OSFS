# Heap card

- One malloc -> one free, exactly once. Null the pointer after.
- Check every return: `NULL` (malloc/realloc) means OOM path, not crash path.
- `realloc` may MOVE: use the new pointer only; never keep the old alias.
- Prove it: `cc -fsanitize=address,undefined` + valgrind for leaks. "Runs fine" is not evidence.

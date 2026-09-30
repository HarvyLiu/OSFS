# Ownership card

- Rules: one owner; move transfers (old dies); borrow & (many readers) XOR &mut (one writer).
- Own: String, Vec (drop frees). View: &str, &[T] (ptr+len, no free). Copy: i32/bool (dup, no move).
- mut is contract: &mut needs let mut. volatile-like honesty, borrow-flavored.
- Debug: rustc --explain E0382/E0502; read full errors, they teach.
- Build: rustc --edition 2021 (-D warnings); tests: rustc --test.

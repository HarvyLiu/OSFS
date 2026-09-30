# FFI fence pre-flight

- Widths: i32/u32/i64/u64 ONLY across. Never long/int/char (platform wobble).
- Rust side: #[repr(C)] + #[no_mangle] + extern "C". C side: stdint mirror + extern decl.
- Check: rust sizes/offsets == C offsetof (test both, compare numbers, not hopes).
- nm: T delivers / U promises. Both must name the SAME plain symbol.
- Link: ONE matching target (kernel build). Mixed toolchains fail honestly.
- Panic: kernels use panic="abort" (no unwinder below). unsafe at fence + SAFETY notes.

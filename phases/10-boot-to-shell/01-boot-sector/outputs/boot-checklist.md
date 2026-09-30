# Boot checklist (every stage 10/02+ re-verifies)

- .code16 (real-mode encodings, not host-64).
- cli first, segs zero (DS/SS), stack below load (0x7C00 down), sti after.
- Origin: -Ttext 0x7c00 ($msg absolute right).
- Budget: .fill to 510 (oversize = build error), .word 0xAA55 at 510-511.
- Output: VGA int10 (humans) + COM1 serial (CI nographic).
- Run: timeout 5 qemu -drive format=raw -nographic; 124 = success.
- Prove: 512 bytes, 55aa tail, message bytes, cli first (test_boot.py).

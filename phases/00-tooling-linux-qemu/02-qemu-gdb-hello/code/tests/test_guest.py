"""test_guest.py — 3 static checks on the bare-metal guest (no QEMU needed)."""
import re
import unittest
from pathlib import Path

CODE = Path(__file__).resolve().parents[1]


class TestGuest(unittest.TestCase):
    def _read(self, name: str) -> str:
        return (CODE / name).read_text(encoding="utf-8")

    def test_multiboot_magic(self):
        text = self._read("boot.s")
        self.assertIn("0x1BADB002", text)
        self.assertIn("CHECKSUM", text)

    def test_serial_port(self):
        text = self._read("kernel.c")
        self.assertIn("0x3F8", text)
        self.assertIn("outb", text)
        self.assertIn("0x20", text)  # THR-empty wait

    def test_linker_entry(self):
        text = self._read("linker.ld")
        self.assertIn("ENTRY(_start)", text)
        self.assertIn("1M", text)


if __name__ == "__main__":
    unittest.main()

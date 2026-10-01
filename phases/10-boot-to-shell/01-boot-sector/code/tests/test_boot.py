"""test_boot.py -- 5 structural checks on boot.bin (no QEMU needed)."""
import sys
import unittest
from pathlib import Path

BIN = Path(__file__).resolve().parents[1] / "build" / "boot.bin"


class TestBoot(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not BIN.exists():
            raise unittest.SkipTest("build/boot.bin absent (Linux-only build); run make first")

    def test_size(self):
        self.assertEqual(BIN.stat().st_size, 512)

    def test_signature(self):
        data = BIN.read_bytes()
        self.assertEqual(data[510:512], b"\x55\xaa")

    def test_message(self):
        self.assertIn(b"OSFS boot!", BIN.read_bytes())

    def test_entry_cli(self):
        self.assertEqual(BIN.read_bytes()[0], 0xFA)  # cli first: fence!

    def test_vga_direct(self):
        self.assertIn(b"\xb8\x00\xb8", BIN.read_bytes())  # mov $0xB800,%ax: cells, not int10
        self.assertNotIn(b"\xcd\x10", BIN.read_bytes())  # no BIOS teletype (sercon echoes!)


if __name__ == "__main__":
    unittest.main(verbosity=2 if "-v" in sys.argv else 1)

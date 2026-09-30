"""test_prot.py -- 5 structural checks on os.bin (no QEMU needed)."""
import sys
import unittest
from pathlib import Path

OSBIN = Path(__file__).resolve().parents[1] / "build" / "os.bin"


class TestProt(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not OSBIN.exists():
            raise unittest.SkipTest("build/os.bin absent (Linux-only build); run make first")

    def test_size_aligned(self):
        self.assertEqual(OSBIN.stat().st_size % 512, 0)

    def test_boot_signature(self):
        self.assertEqual(OSBIN.read_bytes()[510:512], b"\x55\xaa")

    def test_kernel_message(self):
        self.assertIn(b"protected! C runs.", OSBIN.read_bytes())

    def test_gdt_code_segment(self):
        self.assertIn(b"\x9a\xcf", OSBIN.read_bytes()[:512])  # access+granularity

    def test_entry_cli(self):
        self.assertEqual(OSBIN.read_bytes()[0], 0xFA)  # cli first, again


if __name__ == "__main__":
    unittest.main(verbosity=2 if "-v" in sys.argv else 1)

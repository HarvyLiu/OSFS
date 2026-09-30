"""test_shell.py -- 6 structural checks on os.bin (no QEMU needed)."""
import sys
import unittest
from pathlib import Path

OSBIN = Path(__file__).resolve().parents[1] / "build" / "os.bin"


class TestShell(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not OSBIN.exists():
            raise unittest.SkipTest("build/os.bin absent (Linux-only build); run make first")

    def test_size_aligned(self):
        self.assertEqual(OSBIN.stat().st_size % 512, 0)

    def test_boot_signature(self):
        self.assertEqual(OSBIN.read_bytes()[510:512], b"\x55\xaa")

    def test_banner(self):
        self.assertIn(b"myos> paging+timer+tasks+shell", OSBIN.read_bytes())

    def test_paging_msg(self):
        self.assertIn(b"paging on", OSBIN.read_bytes())

    def test_fs_content(self):
        self.assertIn(b"hello from myos fs", OSBIN.read_bytes())

    def test_entry_cli(self):
        self.assertEqual(OSBIN.read_bytes()[0], 0xFA)


if __name__ == "__main__":
    unittest.main(verbosity=2 if "-v" in sys.argv else 1)

"""test_verify.py — 3 checks for the toolchain checker itself."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import verify  # noqa: E402


class TestVerify(unittest.TestCase):
    def test_probe_returns_string(self):
        self.assertIsInstance(verify.probe("python3"), str)

    def test_required_list_covers_core(self):
        for tool in ["gcc", "make", "gdb", "qemu-system-x86_64", "rustc"]:
            self.assertIn(tool, verify.REQUIRED)

    def test_strict_adds_i386(self):
        self.assertIn("qemu-system-i386", verify.STRICT_EXTRA)


if __name__ == "__main__":
    unittest.main()

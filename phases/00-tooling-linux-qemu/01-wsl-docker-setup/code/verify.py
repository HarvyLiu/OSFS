"""verify.py — toolchain presence + version probe. Stdlib only.
Lesson: phases/00-tooling-linux-qemu/01-wsl-docker-setup/docs/en.md
Exit 0 iff all required tools found. --strict also requires qemu-system-i386.
"""
import shutil
import subprocess
import sys

REQUIRED = ["gcc", "make", "gdb", "qemu-system-x86_64", "rustc", "python3"]
STRICT_EXTRA = ["qemu-system-i386"]


def probe(tool: str) -> str:
    try:
        out = subprocess.run(
            [tool, "--version"], capture_output=True, text=True, timeout=10
        )
        first = (out.stdout or out.stderr or "").strip().splitlines()
        return first[0][:120] if first else "found (no --version output)"
    except Exception as e:  # noqa: BLE001 - report anything as version string
        return f"error probing: {e}"


def main(argv) -> int:
    tools = list(REQUIRED)
    if "--strict" in argv:
        tools += STRICT_EXTRA
    missing = [t for t in tools if shutil.which(t) is None]
    for t in tools:
        status = "MISSING" if t in missing else probe(t)
        print(f"{t}: {status}")
    if missing:
        print(f"missing: {missing}")
        print("fix (WSL/Ubuntu): sudo apt install -y qemu-system-x86 gdb gcc-multilib make")
        return 1
    print("toolchain OK")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

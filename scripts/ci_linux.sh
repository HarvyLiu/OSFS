#!/bin/sh
# ci_linux.sh -- full Linux verification inside the Docker toolchain image.
# Runs: sync, all three audits, every host `make test`, boot-image builds
# for 10/01-03, and banner expects on QEMU serial. Fails loudly on any miss.
set -eu

python3 scripts/sync_site.py
python3 scripts/audit_lessons.py
python3 scripts/check_figures.py
python3 scripts/build_graph.py --check

fails=0
for mk in $(grep -rl '^test:' phases/*/*/code/Makefile); do
  d=$(dirname "$mk")
  echo "== make test in $d"
  make -C "$d" test || fails=1
done

make -C phases/10-boot-to-shell/01-boot-sector/code
make -C phases/10-boot-to-shell/02-protected-mode/code
make -C phases/10-boot-to-shell/03-shell-capstone/code

timeout 5 qemu-system-x86_64 -drive format=raw,file=phases/10-boot-to-shell/01-boot-sector/code/build/boot.bin -nographic | grep -q 'OSFS boot!'
echo "boot 10/01 banner OK"
timeout 5 qemu-system-x86_64 -drive format=raw,file=phases/10-boot-to-shell/02-protected-mode/code/build/os.bin -nographic | grep -q 'protected! C runs.'
echo "boot 10/02 banner OK"
timeout 5 qemu-system-x86_64 -drive format=raw,file=phases/10-boot-to-shell/03-shell-capstone/code/build/os.bin -nographic | grep -q 'myos>'
echo "boot 10/03 banner OK"

test "$fails" -eq 0
echo "LINUX-ALL-OK"

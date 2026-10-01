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

boot_check() {
  img=$1; banner=$2
  log=/tmp/osfs-boot-$(echo "$img" | tr '/' '-').log
  echo "== boot $img (expect: $banner)"
  timeout 20 qemu-system-x86_64 -drive format=raw,file="$img" -nographic > "$log" 2>&1 || rc=$?
  cat "$log"
  test "${rc:-0}" -eq 124
  grep -q "$banner" "$log"
  echo "banner OK: $banner"
}
boot_check phases/10-boot-to-shell/01-boot-sector/code/build/boot.bin 'OSFS boot!'
boot_check phases/10-boot-to-shell/02-protected-mode/code/build/os.bin 'protected! C runs.'
boot_check phases/10-boot-to-shell/03-shell-capstone/code/build/os.bin 'myos>'

test "$fails" -eq 0
echo "LINUX-ALL-OK"

"""check_figures.py — every .svg needs a same-basename .excalidraw source."""
import sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
errs = []
svgs = list((ROOT / "phases").rglob("figures/*.svg")) if (ROOT / "phases").exists() else []
for svg in svgs:
    # strip .svg, handle .dark.svg
    base = svg.name.replace(".dark.svg", "").replace(".light.svg", "").replace(".svg", "")
    ex = svg.parent / f"{base}.excalidraw"
    if not ex.exists():
        errs.append(f"{svg}: missing {ex.name} source (commit editable Excalidraw JSON)")
if errs:
    print(f"FAIL: {len(errs)} figure(s):")
    for e in errs: print(" -", e)
    sys.exit(1)
print(f"OK: {len(svgs)} svg(s) have excalidraw sources")

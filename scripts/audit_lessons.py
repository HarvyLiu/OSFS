"""audit_lessons.py — enforce beginner + codeblock + frontmatter contract."""
import re, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PHASES = ROOT / "phases"
ERRORS = []

FRONTMATTER_KEYS = ["**Type:**", "**Languages:**", "**Prerequisites:**", "**College ref:**", "**Time:**"]
REQUIRED_SECTIONS = ["## Concept", "## Simulate", "## Build", "## Use It", "## Ship It", "## Exercises", "## Key Terms"]

def check_lesson(en_path: Path):
    text = en_path.read_text(encoding="utf-8", errors="ignore")
    for k in FRONTMATTER_KEYS:
        if k not in text:
            ERRORS.append(f"{en_path}: missing frontmatter {k}")
    for s in REQUIRED_SECTIONS:
        if s not in text:
            ERRORS.append(f"{en_path}: missing section {s}")
    # every opening fenced block needs language tag (closings are bare ```)
    in_code = False
    for m in re.finditer(r"^```(\w*).*$", text, re.M):
        tag = m.group(1).strip()
        if not in_code:
            if not tag:
                ERRORS.append(f"{en_path}: fenced block missing language tag")
            in_code = True
        else:
            in_code = False
    # every code block needs a 'What this does' within next 8 lines after close
    blocks = list(re.finditer(r"```\w+\n(.*?)```", text, re.S))
    for b in blocks:
        tail = text[b.end():b.end()+1200]
        if "What this does" not in tail[:1200]:
            ERRORS.append(f"{en_path}: code block without 'What this does' explanation")
            break
    # ASM: flag Intel-only syntax (mov eax, 1 without %/$) as warning->error if no AT&T markers nearby
    if "asm" in text.lower():
        # crude: if lesson has ASM block with 'mov eax' and no '%' anywhere, complain
        for b in blocks:
            code = b.group(1)
            if re.search(r"\bmov\s+eax\b", code, re.I) and "%" not in code:
                ERRORS.append(f"{en_path}: ASM looks Intel-only; use AT&T GAS (%reg, $imm) with side-table")
                break

def main():
    if not PHASES.exists():
        print("no phases/ yet, ok"); return 0
    lessons = sorted(PHASES.rglob("docs/en.md"))
    if not lessons:
        print("no lessons found"); return 0
    for p in lessons:
        check_lesson(p)
    if ERRORS:
        print(f"FAIL: {len(ERRORS)} issue(s):")
        for e in ERRORS[:50]:
            print(" -", e)
        return 1
    print(f"OK: {len(lessons)} lessons pass audit")
    return 0

if __name__ == "__main__":
    sys.exit(main())

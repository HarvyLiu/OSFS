"""sync_site.py -- generate Starlight pages from lessons (idempotent, stdlib only).

Reads phases/<phase>/<lesson>/docs/en.md (+ figures/*.svg), writes
site-astro/src/content/docs/lessons/<phase>--<lesson>.md with SVGs alongside.
Also syncs ROADMAP.md -> docs/roadmap.md and glossary/terms.md -> docs/glossary.md.

Generated paths are gitignored (see .gitignore); `npm run dev/build` triggers
this via predev/prebuild, CI runs it before `astro build`.
"""
import re
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PHASES = ROOT / "phases"
SITE = ROOT / "site-astro" / "src" / "content" / "docs"
LESSONS_DIR = SITE / "lessons"

# Pedagogical order (rglob sorts P- last; the book reads primer first).
PHASE_ORDER = [
    "P-00-mental-model", "P-01-c-from-zero", "P-02-asm-from-zero",
    "P-03-rust-from-zero", "00-tooling-linux-qemu", "01-what-is-os",
    "02-processes", "03-scheduling", "04-concurrency", "05-memory-i",
    "06-memory-ii", "07-persistence", "08-filesystems", "09-protection",
    "10-boot-to-shell",
]
PART_LABELS = {
    "P-00-mental-model": "Primer · Mental Models",
    "P-01-c-from-zero": "Primer · C",
    "P-02-asm-from-zero": "Primer · Assembly",
    "P-03-rust-from-zero": "Primer · Rust",
    "00-tooling-linux-qemu": "Tooling",
    "01-what-is-os": "Part I · What Is an OS",
    "02-processes": "Part II · Processes",
    "03-scheduling": "Part III · Scheduling",
    "04-concurrency": "Part IV · Concurrency",
    "05-memory-i": "Part V · Memory I",
    "06-memory-ii": "Part VI · Memory II",
    "07-persistence": "Part VII · Persistence",
    "08-filesystems": "Part VIII · Filesystems",
    "09-protection": "Part IX · Protection",
    "10-boot-to-shell": "Part X · Capstone",
}


def frontmatter(title, description):
    def esc(s):
        return s.replace("\\", "\\\\").replace('"', "'").replace("\n", " ")
    return f'---\ntitle: "{esc(title)}"\ndescription: "{esc(description)}"\n---\n\n'


def split_title_hook(text):
    title, hook = "Untitled", ""
    for line in text.splitlines():
        if title == "Untitled" and line.startswith("# "):
            title = line[2:].strip()
        elif not hook and line.startswith("> "):
            hook = line[2:].strip()
        if title != "Untitled" and hook:
            break
    return title, hook


def strip_first_h1(text):
    lines = text.splitlines(keepends=True)
    for i, line in enumerate(lines):
        if line.startswith("# "):
            del lines[i]
            break
    return "".join(lines).lstrip("\n")


def lesson_time(text):
    m = re.search(r"\*\*Time:\*\*\s*(.+)", text)
    return m.group(1).strip() if m else ""


def chapter_kicker(number, phase, text):
    part = PART_LABELS.get(phase, phase)
    time = lesson_time(text) or "self-paced"
    words = len(text.split())
    return (
        f'<p class="chapter-kicker">Chapter {number} · {part} · '
        f'{time} · ~{words} words<br/>╌╌╌╌</p>\n\n'
    )


CHAPTER_END = '\n\n<p class="chapter-end">╌╌ END ╌╌</p>\n'


def sync_lesson(en_path, number):
    phase = en_path.parents[2].name
    lesson = en_path.parents[1].name
    slug = f"{phase}--{lesson}"
    text = en_path.read_text(encoding="utf-8")
    title, hook = split_title_hook(text)
    body = strip_first_h1(text)
    # relocate figures: ../figures/X.svg -> ./<slug>--X.svg (copied alongside)
    fig_src = en_path.parents[1] / "figures"
    if fig_src.is_dir():
        for svg in sorted(fig_src.glob("*.svg")):
            dest_name = f"{slug}--{svg.name}"
            shutil.copyfile(svg, LESSONS_DIR / dest_name)
            body = body.replace(f"](../figures/{svg.name})", f"](./{dest_name})")
    # glossary links: repo-relative -> site page (anchors survive: ## pointer)
    body = body.replace("](../../../../glossary/terms.md", "](../glossary/")
    (LESSONS_DIR / f"{slug}.md").write_text(
        frontmatter(title, hook or title)
        + chapter_kicker(number, phase, text)
        + body.rstrip("\n")
        + CHAPTER_END,
        encoding="utf-8",
    )
    return slug


def sync_page(src, dest_name, fallback_title):
    text = src.read_text(encoding="utf-8")
    title, hook = split_title_hook(text)
    body = strip_first_h1(text)
    (SITE / dest_name).write_text(
        frontmatter(title or fallback_title, hook or fallback_title) + body,
        encoding="utf-8",
    )


def ordered_lessons():
    found = list(PHASES.rglob("docs/en.md")) if PHASES.exists() else []
    by_phase = {}
    for en in found:
        by_phase.setdefault(en.parents[2].name, []).append(en)
    ordered = []
    for phase in PHASE_ORDER:
        ordered += sorted(by_phase.get(phase, []))
    return ordered


def main():
    if LESSONS_DIR.exists():
        shutil.rmtree(LESSONS_DIR)
    LESSONS_DIR.mkdir(parents=True)
    lessons = ordered_lessons()
    for n, en in enumerate(lessons, 1):
        sync_lesson(en, n)
    sync_page(ROOT / "ROADMAP.md", "roadmap.md", "Roadmap")
    sync_page(ROOT / "glossary" / "terms.md", "glossary.md", "Glossary")
    print(f"OK: synced {len(lessons)} lessons + roadmap + glossary to site-astro")
    return 0


if __name__ == "__main__":
    sys.exit(main())

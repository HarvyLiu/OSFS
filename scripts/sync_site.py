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


def sync_lesson(en_path):
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
        frontmatter(title, hook or title) + body, encoding="utf-8"
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


def main():
    if LESSONS_DIR.exists():
        shutil.rmtree(LESSONS_DIR)
    LESSONS_DIR.mkdir(parents=True)
    lessons = sorted(PHASES.rglob("docs/en.md")) if PHASES.exists() else []
    for en in lessons:
        sync_lesson(en)
    sync_page(ROOT / "ROADMAP.md", "roadmap.md", "Roadmap")
    sync_page(ROOT / "glossary" / "terms.md", "glossary.md", "Glossary")
    print(f"OK: synced {len(lessons)} lessons + roadmap + glossary to site-astro")
    return 0


if __name__ == "__main__":
    sys.exit(main())

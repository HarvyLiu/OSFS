# AGENTS.md

Operating manual for contributors and AI agents. Read before any PR.

This repo is a curriculum, not an app. Lessons are the product. Beginners are the audience.

## Philosophy

Concept first, hands-on second. Every lesson: 60-second diagram → host simulator → QEMU bare-metal → Linux observation (`strace`/`/proc`) → shipped artifact. Every codeblock explained line-by-line. No magic.

Linux-first: WSL2 / native Linux canonical. Must also pass in `docker/Dockerfile`. Windows-native is best-effort.

## Repo layout

```text
phases/<NN>-slug/<MM>-slug/
  docs/en.md
  code/            # Makefile + main.c/*.s/*.rs + tests/
  figures/*.excalidraw + *.svg
  quiz.json        # 1 pre + 3 check + 2 post
  outputs/         # runbook / template / cheatsheet
```

## Hard rules

1. **One commit per lesson dir.** Conventional subject ≤72 chars: `feat(phase-MM): <slug>`.
2. **AT&T (GAS) canonical for ASM.** Intel shown only in side-table. Every ASM block uses `%reg`, `$imm`, `src → dst` order. Explain suffixes (`b/w/l/q`).
3. **Every fenced code block needs:** language tag + `What this does` (1–2 sentences) + line table `| Lines | Code | Why |` + one `Change X → Y` experiment + verify command + expected output. Audit fails otherwise.
4. **Mermaid or Excalidraw-SVG only.** No ASCII box art. Every `.svg` in `figures/` must have a same-basename `.excalidraw` source. `check_figures.py` gates this.
5. **Define-then-use.** First use of pointer, address, register, stack, heap, syscall, PCB, page, TLB, inode, journal links to `glossary/terms.md`. No forward refs — `build_graph.py` checks `Prerequisites` DAG.
6. **Stdlib-first, freestanding-aware.** Host simulators: C stdlib + Rust std allowed. Bare-metal `kernel.*`: no hosted libc, document `-ffreestanding`, linker script, `panic_handler`.
7. **Never commit generated files:** `site-astro/dist/`, `node_modules/`. Commit `.excalidraw` + `.svg`, not PNG exports.
8. **Original prose + cited refs.** Don't copy OSTEP/xv6 text. Cite `OSTEP Ch.X, MIT 6.1810 Lec Y, xv6 file Z` in frontmatter.

## Lesson contract — docs/en.md frontmatter

```markdown
# <Title>

> <One-line hook>

**Type:** Learn | Build
**Languages:** C, ASM, Rust (subset actually used)
**Prerequisites:** <slugs or None>
**College ref:** OSTEP Ch.X, MIT 6.1810 Lec Y, xv6 file Z
**Time:** ~<minutes>
```

Required sections: `Concept in 60s`, `Simulate It`, `Build It`, `Use It (Linux)`, `Ship It`, `Exercises`, `Key Terms`.

## Code contract

- Host sim must build with `cc -Wall -Werror` or `cargo test` and exit 0, no QEMU needed.
- Bare-metal must build with `make` and boot in `qemu-system-x86_64 -nographic` with serial output, timeout-safe.
- 4–6 line header comment citing `docs/en.md` path.
- `code/tests/` with ≥3 checks (C `assert` runner or `cargo test`).
- Tests run from `code/` as cwd (`make -C <lesson>/code test` → `python3 -m unittest discover -s tests -v`). Test files must `read_text(encoding="utf-8")` and fix `sys.path` to import sibling modules — never rely on repo-root cwd or locale default encoding.
- Code sources (`*.c/*.s/*.rs/*.py`) stay plain ASCII in comments/strings where possible (no em-dashes/smart quotes); docs (`en.md`) may use full UTF-8.

## quiz.json schema

```json
{
  "lesson": "<dir-slug>",
  "title": "<Title>",
  "questions": [
    {"stage": "pre", "question": "...", "options": ["a","b","c","d"], "correct": 0, "explanation": ""},
    {"stage": "check", "question": "...", "options": ["a","b","c","d"], "correct": 1, "explanation": ""},
    {"stage": "check", "question": "...", "options": ["a","b","c","d"], "correct": 0, "explanation": ""},
    {"stage": "check", "question": "...", "options": ["a","b","c","d"], "correct": 2, "explanation": ""},
    {"stage": "post", "question": "...", "options": ["a","b","c","d"], "correct": 3, "explanation": ""},
    {"stage": "post", "question": "...", "options": ["a","b","c","d"], "correct": 1, "explanation": ""}
  ]
}
```

## Per-PR validation

```bash
python3 scripts/audit_lessons.py
python3 scripts/check_figures.py
python3 scripts/build_graph.py --check
python3 scripts/sync_site.py   # regenerates gitignored site-astro pages (lessons + roadmap + glossary)
# per lesson touched:
make -C phases/<phase>/<lesson>/code run
```

CI (`.github/workflows/curriculum.yml`): audit + figures + graph blocking; site job (`npm run build`, runs sync via prebuild); Docker toolchain job runs `scripts/ci_linux.sh` (all host tests + 10/01–03 boot builds + serial banner expects).

"""build_graph.py — parse Prerequisites DAG, fail on unknown refs or cycles."""
import re, sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]

def slug_of(en_path: Path) -> str:
    # phases/<phase>/<lesson>/docs/en.md -> <phase>/<lesson>
    return f"{en_path.parents[2].name}/{en_path.parents[1].name}"

def main():
    check = "--check" in sys.argv
    lessons = sorted((ROOT / "phases").rglob("docs/en.md")) if (ROOT / "phases").exists() else []
    known = {slug_of(p) for p in lessons}
    # also allow bare lesson slugs and 'None'
    errs = []
    graph = {}
    for p in lessons:
        text = p.read_text(encoding="utf-8", errors="ignore")
        m = re.search(r"\*\*Prerequisites:\*\*\s*(.+)", text)
        deps = []
        if m:
            raw = m.group(1).strip()
            if raw.lower() != "none":
                deps = [d.strip() for d in raw.split(",") if d.strip()]
        graph[slug_of(p)] = deps
        for d in deps:
            if d not in known and d not in {k.split("/")[1] for k in known}:
                errs.append(f"{slug_of(p)}: unknown prerequisite '{d}'")
    # cycle check (DFS)
    WHITE, GRAY, BLACK = 0, 1, 2
    color = {k: WHITE for k in graph}
    def norm(d):
        # allow short form
        if d in graph: return d
        for k in graph:
            if k.endswith("/" + d): return k
        return d
    stack = []
    def dfs(u):
        color[u] = GRAY
        for d in graph[u]:
            v = norm(d)
            if v not in graph: continue
            if color[v] == GRAY:
                errs.append(f"cycle: {' -> '.join(stack + [u, v])}")
            elif color[v] == WHITE:
                stack.append(u); dfs(v); stack.pop()
        color[u] = BLACK
    for k in graph:
        if color[k] == WHITE:
            dfs(k)
    if check and errs:
        print("FAIL:"); [print(" -", e) for e in errs]; sys.exit(1)
    print(f"OK: {len(graph)} nodes in prereq graph")
    if "--dot" in sys.argv:
        print("digraph osfs {")
        for k, vs in graph.items():
            for v in vs: print(f'  "{v}" -> "{k}";')
        print("}")

if __name__ == "__main__":
    main()

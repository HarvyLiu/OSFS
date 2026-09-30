# Rings + Syscall Gates — Who May Touch What, Enforced in Silicon

> Ring 0 does anything. Ring 3 asks through gates. The CPU checks every crossing — no exceptions, only faults.

**Type:** Learn
**Languages:** C
**Prerequisites:** 02-syscalls-strace
**College ref:** OSTEP Ch.6 (dual-mode + traps), MIT 6.1810 Lec 4 (rings/trapframes), Intel SDM Vol.3 Ch.5 (privilege checks, the source)
**Time:** ~60 minutes

## Learning Objectives
- Trace 5 crossings (user→syscall gate, user→kernel gate, user→kernel data, kernel→user data, user→user) to allow/fault verdicts
- Implement the one rule (`cpl <= dpl` numerically) over gates + segments in C
- Explain CPL/DPL, why `cli`/`inb` fault in userspace, and gates as the only legal doors
- Connect rings to syscalls (the gates' purpose) and to SMEP/SMAP (walls inside the walls, named)

## Concept in 60s

![rings gates](../figures/rings-gates.svg)

<!-- source: ../figures/rings-gates.excalidraw — open in excalidraw.com to redraw -->

Four rings, two used: 0 (kernel, all powers: `cli`, `inb`, page-table writes, CR3) and 3 (user, asking powers). Rule (one!): access allowed iff `cpl <= dpl` *numerically* (low number = high privilege; you may reach *up* toward your own number, never *down* below it). Gate DPL 3 (`syscall` entry): user 3 ≤ 3 ✓ (the door exists for you). Kernel-only gate DPL 0: user 3 ≤ 0 ✗ (#GP fault — P-02/04's userspace-`cli` death, generalized). Kernel data DPL 0: user read faults; kernel (0 ≤ 0/3) reads all. User data DPL 3: everyone reads (0 ≤ 3 ✓, 3 ≤ 3 ✓ — sharing downward is safe). `syscall`/`sysret` are fast gates (MSRs hold entry, no IDT walk); `int` gates are the slow teaching path.

## Simulate It (host C — the rule, no QEMU)

Full program: `code/rings.c`. Gate table + segment table + verdict matrix.

```c
#include <stdio.h>

#define RING_KERNEL 0
#define RING_USER 3

typedef struct { const char *name; int dpl; int target; } gate_t;
typedef struct { const char *name; int dpl; } seg_t;

// THE rule: numerically cpl <= dpl. Lower number = more privilege.
static int gate_call(int cpl, const gate_t *g) { return cpl <= g->dpl ? 0 : -1; }
static int seg_access(int cpl, const seg_t *s) { return cpl <= s->dpl ? 0 : -1; }

int main(void) {
    gate_t syscall_gate = {"syscall", 3, RING_KERNEL};
    gate_t konly_gate = {"kdebug", 0, RING_KERNEL};
    seg_t kdata = {"kdata", 0};
    seg_t udata = {"udata", 3};
    int v[5];
    v[0] = gate_call(RING_USER, &syscall_gate);   // user->syscall: ok
    v[1] = gate_call(RING_USER, &konly_gate);     // user->kdebug: FAULT
    v[2] = seg_access(RING_USER, &kdata);         // user reads kdata: FAULT
    v[3] = seg_access(RING_KERNEL, &udata);       // kernel reads udata: ok
    v[4] = seg_access(RING_USER, &udata);         // user reads udata: ok
    printf("syscall-gate=%s kdebug-gate=%s kdata=%s k-reads-u=%s u-reads-u=%s\n",
           v[0] == 0 ? "ok" : "FAULT", v[1] == 0 ? "ok" : "FAULT",
           v[2] == 0 ? "ok" : "FAULT", v[3] == 0 ? "ok" : "FAULT",
           v[4] == 0 ? "ok" : "FAULT");
    return !(v[0] == 0 && v[1] == -1 && v[2] == -1 && v[3] == 0 && v[4] == 0);
}
```

What this does: renders the five canonical verdicts from one comparison — the privilege system as a truth table.

| Lines | Code | Why it exists |
|---|---|---|
| 3–4 | ring constants | only 0/3 used in practice (1/2 exist, ignored — history's empty rooms, named so `2` in a dump doesn't mystify) |
| 6–7 | gate vs seg | gate = callable door (dpl + target ring); seg = readable region (dpl only) — two shapes, one rule |
| 10–11 | THE rule twice | `<=` numerically (not `>=`! — the direction beginners flip; the matrix below is the proof) |

Change X → Y: flip `<=` to `>=` in `gate_call` only. Verify: user→kdebug prints `ok` (the catastrophe — a userspace-reachable debug gate; the one-character bug that ends kernels; revert with shaking hands).

## Build It

```bash
make run
make test
```

What this does: prints `ok FAULT FAULT ok ok`, then asserts the matrix + boundary (ring 3 vs DPL 3 passes — equality allowed: doors open *at* your level).

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | five verdicts, exact order (memorize the shape: only downward crossings fault) |
| `make test` | machine proof | matrix asserts + `cpl==dpl` boundary + kernel-omnipotence row (0 passes all) |

Change X → Y: add gate `{"syscall", 2, 0}` (DPL 2, the unused ring). Verify: user 3 faults (3 ≤ 2 false — the empty rooms still enforce; DPL means what it says at every level).

## Use It (Linux)

Rings are invisible from userspace *by design* — observe the shadows:

```bash
./build/ringsdemo
dmesg 2>/dev/null | grep -iE "segfault|general protection" | head -3 || echo "(quiet: no recent #GP/#PF diary entries)"
cat /proc/self/status | grep -i "seccomp\|NoNewPrivs" | head -3
```

What this does: runs the matrix, checks the kernel diary for real faults, and shows your sandboxing state (seccomp = *fewer* syscalls allowed — gates with a bouncer).

| Lines | Code | Why it exists |
|---|---|---|
| `dmesg segfault` | fault diary | real #GP/#PF lines (address + error code — the silicon enforcing, logged) |
| `seccomp` | gate narrowing | even allowed gates can be filtered per-process (defense in depth: rings, then caps, then seccomp — 09/02 continues the stack) |

Change X → Y: run a userspace `cli` (inline ASM from P-02/05 in a scratch file!). Verify: `SIGSEGV` + `dmesg` line (the P-02/04 warning, now self-inflicted and fully understood — science with a crash helmet).

## Ship It

Artifact: `outputs/rings-card.md` — THE rule, CPL/DPL glossary, gate-vs-segment shapes, `syscall` vs `int` note, SMEP/SMAP one-liners (kernel can't execute/read user memory accidentally — walls inside walls). Protection reference page one.

## Exercises

1. Easy — add ring-1/ring-2 callers to the matrix (predict with THE rule first: `1<=0`? `2<=3`? — empty rooms, enforced rules).
2. Medium — `RPL` twist: effective rule uses `max(CPL,RPL)` (segments carry requestor level — implement `seg_access_rpl(cpl, rpl, seg)`, show a confused-deputy blocked: kernel tricked with user pointer... fails *unless* it validates — the hardening moral).
3. Hard — `SMEP`/`SMAP` flags in the model (kernel executing user code → fault even at CPL 0): add per-access `is_exec`/`is_user_page` params, re-derive the matrix (defense-in-depth quantified: rings + maps + flags, three independent denials).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| syscall | the DPL-3 gate every lesson has knocked (now with its doorframe visible) | ../../glossary/terms.md#syscall |
| register | CPL lives in CS register bits (RPL in selector — silicon-stored privilege) | ../../glossary/terms.md#register |
| address | DPLs attach to segments/pages (protection rides addressing — 05 meets 09) | ../../glossary/terms.md#address |

## Further Reading

- Intel SDM Vol.3 Ch.5 — privilege checks (the 10 pages this lesson compresses to one rule).
- MIT 6.1810 traps lecture (rings half) + `man 2 syscall` (the gate you already use).
- `man 7 seccomp` + `Documentation/security/` (gates with bouncers — 09/02's neighborhood).

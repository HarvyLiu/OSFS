# Users, Groups, Capabilities — Names on the Doors

> Rings say how strong you are. Uids say who you are. Capabilities say exactly what you may do.

**Type:** Learn
**Languages:** C
**Prerequisites:** 01-rings-gates
**College ref:** OSTEP Ch.6 (protectionatrix preview), MIT 6.1810 isolation notes (user/kernel split), Linux `capabilities(7)` (the bitmask that retired root)
**Time:** ~60 minutes

## Learning Objectives
- Trace open() through owner/group/other bits for three users plus root, verdict by verdict
- Implement Unix permission check + capability-gated raw access in C
- Explain setuid (borrowed identity), groups (shared doors), and caps (root split into bits)
- Connect `EACCES` to rings (who) + perms (whose) + caps (may-what): the full denial stack

## Concept in 60s

![perms caps](../figures/perms-caps.svg)

<!-- source: ../figures/perms-caps.excalidraw — open in excalidraw.com to redraw -->

Every process carries uid+gid (who) and cap bitmask (may-what). Every file carries owner-uid + group-gid + 9 mode bits (`rwxr-x---` = 0750: owner all, group read+enter, others nothing). `open` for write: root (uid 0)? always yes (the skeleton key — capabilities retire it piece by piece). Else pick the *first* matching class (owner? else group? else other — first match wins, no OR-ing down) and test the bit. Raw disk (`/dev/sda`) additionally demands `CAP_SYS_RAWIO` (even rightful owners kneel without the bit — defense in depth: identity *and* capability). setuid binaries run as the *file's* owner (borrowed root for `passwd`'s minute — the sharpest door in Unix, audited accordingly).

## Simulate It (host C — the matrix, no QEMU)

Full program: `code/perms.c`. One file (0640, alice:staff), four askers, one raw disk.

```c
#include <stdio.h>

#define CAP_DISK 0x1u

typedef struct { int uid; int gid; unsigned caps; const char *who; } proc_t;
typedef struct { int owner; int group; unsigned mode; const char *name; } file_t;

static int unix_open(const proc_t *p, const file_t *f, int want_write) {
    if (p->uid == 0) return 0;                       // root: skeleton key
    unsigned bits;
    if (p->uid == f->owner) bits = (f->mode >> 6) & 7;
    else if (p->gid == f->group) bits = (f->mode >> 3) & 7;
    else bits = f->mode & 7;                         // first match wins
    unsigned need = want_write ? 2 : 4;
    return (bits & need) ? 0 : -1;
}

static int raw_open(const proc_t *p) {
    if (p->uid != 0 && !(p->caps & CAP_DISK)) return -1;  // identity AND capability
    return 0;
}

int main(void) {
    file_t note = {1000, 100, 0640, "note"};
    proc_t alice = {1000, 100, 0, "alice"};
    proc_t bob = {101, 100, 0, "bob"};
    proc_t eve = {200, 200, 0, "eve"};
    proc_t root = {0, 0, 0, "root"};
    proc_t op = {200, 200, CAP_DISK, "op-with-cap"};
    int v[6];
    v[0] = unix_open(&alice, &note, 1);  // owner write: ok
    v[1] = unix_open(&bob, &note, 1);    // group write? 0640 group=r--: DENY
    v[2] = unix_open(&bob, &note, 0);    // group read: ok
    v[3] = unix_open(&eve, &note, 0);    // other: DENY
    v[4] = unix_open(&root, &note, 1);   // root: ok
    v[5] = raw_open(&op);                // cap, non-root: ok
    printf("alice-w=%s bob-w=%s bob-r=%s eve-r=%s root-w=%s op-raw=%s\n",
           v[0]==0?"ok":"DENY", v[1]==0?"ok":"DENY", v[2]==0?"ok":"DENY",
           v[3]==0?"ok":"DENY", v[4]==0?"ok":"DENY", v[5]==0?"ok":"DENY");
    int eve_raw = raw_open(&eve);
    printf("eve-raw=%s (no cap, non-root)\n", eve_raw==0?"ok":"DENY");
    return !(v[0]==0 && v[1]==-1 && v[2]==0 && v[3]==-1 && v[4]==0 && v[5]==0 && eve_raw==-1);
}
```

What this does: renders Unix's first-match-wins plus capability-gating as seven verdicts — the denial stack end to end.

| Lines | Code | Why it exists |
|---|---|---|
| 11 | root early-yes | uid 0 bypasses (skeleton key — capabilities exist to *retire* this line case by case) |
| 12–15 | first-match class | owner? group? other? — exactly one applies (bob is *in* staff: group bits, even though "other" would deny — order matters, OR-ing down forbidden) |
| 16–17 | bit test | write=2, read=4 (execute=1 for dirs/programs — same trio, third bit) |
| 20–23 | raw gate | non-root needs the *bit* (identity insufficient — depth: rings say ring, perms say whose, caps say may-what) |
| 31–36 | matrix | owner-ok / group-write-DENY / group-read-ok / other-DENY / root-ok / cap-ok (0640's story told completely) |

Change X → Y: change mode `0640` → `0660`. Verify: `bob-w` flips DENY→ok (group gains write — one octal digit moves a whole class; modes are data, re-read them per file, never memorized globally).

## Build It

```bash
make run
make test
```

What this does: prints the seven verdicts, then asserts the matrix + setuid-borrow + cap-without-identity-denied.

| Lines | Code | Why it exists |
|---|---|---|
| `make run` | human proof | expect `alice-w=ok bob-w=DENY bob-r=ok eve-r=DENY root-w=ok op-raw=ok` + `eve-raw=DENY` |
| `make test` | machine proof | same functions, asserted (permission logic is security: pin every cell) |

Change X → Y: make eve uid 0 (root twins). Verify: all-ok (the skeleton key's blast radius in one edit — why capabilities split it into bits).

## Use It (Linux)

Your own identity, live:

```bash
id
ls -l /etc/shadow 2>&1 | head -2 || echo "(shadow unreadable: good)"
ls -l $(which passwd)
```

What this does: prints your uid/gids, proves shadow denies you (0640 root:shadow — you're neither), and shows `passwd`'s setuid bit (`rws` — borrowed root, the sharpest door, audited).

| Lines | Code | Why it exists |
|---|---|---|
| `id` | whoami once | uid/gid/groups = your process's three askers (every denial below references these numbers) |
| `/etc/shadow` | denial observed | `EACCES` in the wild (first-match: you're other, mode 640 other=--- — the matrix, production) |
| `rws passwd` | borrowed root | setuid: runs as *file owner* (root) briefly (02-setuid-lesson's whole topic in 3 letters: `s`) |

Change X → Y: `capsh --print 2>/dev/null | head -5 || echo no-capsh`. Verify: your *bounding set* listed (capabilities you could ever gain — least-privilege, enumerated).

## Ship It

Artifact: `outputs/perms-card.md` — mode-bit reading (rwx × ugo, octal fluency), first-match rule, root/cap/setuid trio, `id`/`ls -l`/`capsh` verbs. Protection reference complete (09: rings + names + bits).

## Exercises

1. Easy — add execute checks (`./prog` needs `x` on file + dirs): predict `bob-x` on 0750 vs 0640 (bit trio complete).
2. Medium — setuid simulator: `run_as(file_owner, caller)` wrapper granting file-owner uid for one call; show eve writing via setuid-alice-binary, denied without (borrowed identity, fenced).
3. Hard — capability-drop demo: start with all caps, drop all but one per phase (init → serve loop keeps only needed): enumerate each drop (least privilege as a *sequence*, not a state — the hardening discipline).

## Key Terms

| Term | Plain meaning | Link |
|---|---|---|
| syscall | `open` enforces all three layers (rings cross + perms check + cap test) | ../../glossary/terms.md#syscall |
| heap | credential structs live per-process (kernel memory, user read-only via /proc) | ../../glossary/terms.md#heap |
| PCB | carries uid/gid/caps (identity rides the card from 02/01) | ../../glossary/terms.md#pcb |

## Further Reading

- `man 7 credentials` + `man 7 capabilities` (the two manuals this lesson runs).
- `man 2 open` (EACCES rows) + `man 1 chmod` (octal fluency drills).
- OSTEP security bits + Linux `Documentation/security/` (setuid audit guidance).

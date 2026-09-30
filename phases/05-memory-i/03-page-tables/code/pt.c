// pt.c -- walk with on-demand tables. Lesson docs/en.md.
#include <stdlib.h>
#include "pt.h"

pdir_t *pt_new(void) {
    pdir_t *r = calloc(1, sizeof *r);
    return r;
}

void pt_free(pdir_t *root) {
    if (!root) return;
    for (int i = 0; i < PT_ENTRIES; i++) free(root->dir[i]);
    free(root);
}

static ptable_t *get_table(pdir_t *root, uint32_t va, int create) {
    uint32_t pd = (va >> 22) & 0x3FF;
    if (!root->dir[pd]) {
        if (!create) return 0;
        root->dir[pd] = calloc(1, sizeof(ptable_t));
        if (!root->dir[pd]) return 0;
    }
    return root->dir[pd];
}

int pt_map(pdir_t *root, uint32_t va, uint32_t pa, uint32_t flags) {
    ptable_t *t = get_table(root, va, 1);
    if (!t) return -1;
    t->e[(va >> 12) & 0x3FF] = (pa & 0xFFFFF000u) | (flags & 0xFFFu) | PTE_P;
    return 0;
}

int pt_unmap(pdir_t *root, uint32_t va) {
    ptable_t *t = get_table(root, va, 0);
    if (!t) return -2;
    t->e[(va >> 12) & 0x3FF] = 0;
    return 0;
}

int pt_translate(pdir_t *root, uint32_t va, int is_write, uint32_t *pa) {
    ptable_t *t = get_table(root, va, 0);
    if (!t) return -2;
    uint32_t e = t->e[(va >> 12) & 0x3FF];
    if (!(e & PTE_P)) return -3;
    if (is_write && !(e & PTE_RW)) return -4;
    *pa = (e & 0xFFFFF000u) | (va & 0xFFFu);
    return 0;
}

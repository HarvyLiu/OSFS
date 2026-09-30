// pt.h -- 2-level 32-bit tables: 10/10/12. Lesson docs/en.md.
#ifndef OSFS_PT_H
#define OSFS_PT_H

#include <stdint.h>

#define PT_ENTRIES 1024
#define PTE_P 0x1u
#define PTE_RW 0x2u
#define PTE_US 0x4u

typedef struct { uint32_t e[PT_ENTRIES]; } ptable_t;
typedef struct { ptable_t *dir[PT_ENTRIES]; } pdir_t;

pdir_t *pt_new(void);
void pt_free(pdir_t *root);
int pt_map(pdir_t *root, uint32_t va, uint32_t pa, uint32_t flags);
int pt_unmap(pdir_t *root, uint32_t va);
int pt_translate(pdir_t *root, uint32_t va, int is_write, uint32_t *pa);

#endif

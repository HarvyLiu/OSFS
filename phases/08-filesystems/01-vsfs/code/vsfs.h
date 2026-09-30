// vsfs.h -- geometry + ops + verbs. Lesson docs/en.md.
#ifndef OSFS_VSFS_H
#define OSFS_VSFS_H

#include <stddef.h>

#define VSB_NBLOCKS 64
#define VSB_BSIZE 64
#define VSB_NINODES 8
#define VSB_NDIRECT 4
#define VSB_MAXFILE (VSB_NDIRECT * VSB_BSIZE)
#define VSB_NAMELEN 12

typedef struct {
    int (*read)(void *ctx, int bno, unsigned char *out);
    int (*write)(void *ctx, int bno, const unsigned char *in);
    void *ctx;
} vsfs_ops_t;

int vsfs_mkfs(vsfs_ops_t *ops);
int vsfs_create(vsfs_ops_t *ops, const char *name);
int vsfs_write(vsfs_ops_t *ops, int inum, const unsigned char *data, size_t len);
int vsfs_read(vsfs_ops_t *ops, int inum, unsigned char *out, size_t cap);
int vsfs_list(vsfs_ops_t *ops);
int vsfs_used_blocks(vsfs_ops_t *ops);

#endif

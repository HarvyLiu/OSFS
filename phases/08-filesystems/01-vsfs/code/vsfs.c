// vsfs.c -- super, bitmap, inodes, root dir over block ops. Lesson docs/en.md.
#include <stdio.h>
#include <string.h>
#include "vsfs.h"

#define SB_MAGIC 0x76667331u
#define SB_BLOCK 0
#define BITMAP_BLOCK 1
#define INODE_BLOCK0 2
#define INODES_PER_BLOCK (VSB_BSIZE / 32)
#define DATA_BLOCK0 (INODE_BLOCK0 + (VSB_NINODES + INODES_PER_BLOCK - 1) / INODES_PER_BLOCK)
#define T_FILE 1
#define T_DIR 2
#define DENT_PER_BLOCK (VSB_BSIZE / 16)

typedef struct { unsigned type; unsigned size; unsigned direct[VSB_NDIRECT]; unsigned pad[2]; } inode_t;
typedef struct { char name[VSB_NAMELEN]; unsigned inum; } dirent_t;

static int blk_rd(vsfs_ops_t *o, int b, unsigned char *out) { return o->read(o->ctx, b, out); }
static int blk_wr(vsfs_ops_t *o, int b, const unsigned char *in) { return o->write(o->ctx, b, in); }

static int bitmap_set(vsfs_ops_t *o, int bno, int v) {
    unsigned char blk[VSB_BSIZE];
    if (blk_rd(o, BITMAP_BLOCK, blk) != 0) return -1;
    if (v) blk[bno / 8] |= (unsigned char)(1u << (bno % 8));
    else blk[bno / 8] &= (unsigned char)~(1u << (bno % 8));
    return blk_wr(o, BITMAP_BLOCK, blk);
}

static int bitmap_test(vsfs_ops_t *o, int bno) {
    unsigned char blk[VSB_BSIZE];
    if (blk_rd(o, BITMAP_BLOCK, blk) != 0) return -1;
    return (blk[bno / 8] >> (bno % 8)) & 1;
}

static int bitmap_alloc(vsfs_ops_t *o) {
    for (int i = DATA_BLOCK0; i < VSB_NBLOCKS; i++)
        if (bitmap_test(o, i) == 0) {
            if (bitmap_set(o, i, 1) != 0) return -1;
            return i;
        }
    return -1;
}

static int inode_rw(vsfs_ops_t *o, int inum, inode_t *ino, int write) {
    if (inum < 0 || inum >= VSB_NINODES) return -1;
    unsigned char blk[VSB_BSIZE];
    int b = INODE_BLOCK0 + inum / INODES_PER_BLOCK;
    int off = (inum % INODES_PER_BLOCK) * 32;
    if (!write && blk_rd(o, b, blk) != 0) return -1;
    if (write && blk_rd(o, b, blk) != 0) return -1;
    if (write) {
        memcpy(blk + off, ino, 32);
        return blk_wr(o, b, blk);
    }
    memcpy(ino, blk + off, 32);
    return 0;
}

static int check_magic(vsfs_ops_t *o) {
    unsigned char blk[VSB_BSIZE];
    if (blk_rd(o, SB_BLOCK, blk) != 0) return -1;
    unsigned magic;
    memcpy(&magic, blk, 4);
    return magic == SB_MAGIC ? 0 : -1;
}

static int dir_lookup(vsfs_ops_t *o, const char *name, unsigned *inum_out) {
    inode_t root;
    if (inode_rw(o, 0, &root, 0) != 0 || root.type != T_DIR) return -1;
    unsigned char blk[VSB_BSIZE];
    for (int i = 0; i < VSB_NDIRECT; i++) {
        if (root.direct[i] == 0) continue;
        if (blk_rd(o, (int)root.direct[i], blk) != 0) return -1;
        for (int e = 0; e < DENT_PER_BLOCK; e++) {
            dirent_t *de = (dirent_t *)(blk + (size_t)e * 16);
            if (de->name[0] != 0 && strncmp(de->name, name, VSB_NAMELEN) == 0) {
                *inum_out = de->inum;
                return 0;
            }
        }
    }
    return -1;
}

int vsfs_mkfs(vsfs_ops_t *o) {
    unsigned char z[VSB_BSIZE];
    memset(z, 0, sizeof z);
    for (int b = 0; b < VSB_NBLOCKS; b++)
        if (blk_wr(o, b, z) != 0) return -1;
    unsigned char sb[VSB_BSIZE];
    memset(sb, 0, sizeof sb);
    unsigned magic = SB_MAGIC;
    memcpy(sb, &magic, 4);
    memcpy(sb + 4, &(unsigned){VSB_NBLOCKS}, 4);
    memcpy(sb + 8, &(unsigned){VSB_NINODES}, 4);
    if (blk_wr(o, SB_BLOCK, sb) != 0) return -1;
    for (int b = 0; b < DATA_BLOCK0; b++)       // metadata protects itself
        if (bitmap_set(o, b, 1) != 0) return -1;
    inode_t root;
    memset(&root, 0, sizeof root);
    root.type = T_DIR;
    return inode_rw(o, 0, &root, 1);
}

int vsfs_create(vsfs_ops_t *o, const char *name) {
    if (check_magic(o) != 0 || !name || name[0] == 0) return -1;
    if (strlen(name) >= VSB_NAMELEN) return -1;
    unsigned dummy;
    if (dir_lookup(o, name, &dummy) == 0) return -1;  // dupes refused
    inode_t ino;
    int free_inum = -1;
    for (int i = 1; i < VSB_NINODES; i++) {
        if (inode_rw(o, i, &ino, 0) != 0) return -1;
        if (ino.type == 0) { free_inum = i; break; }
    }
    if (free_inum < 0) return -1;
    memset(&ino, 0, sizeof ino);
    ino.type = T_FILE;
    if (inode_rw(o, free_inum, &ino, 1) != 0) return -1;
    inode_t root;
    if (inode_rw(o, 0, &root, 0) != 0) return -1;
    unsigned char blk[VSB_BSIZE];
    for (int i = 0; i < VSB_NDIRECT; i++) {
        if (root.direct[i] == 0) {
            int nb = bitmap_alloc(o);
            if (nb < 0) return -1;
            root.direct[i] = (unsigned)nb;
            memset(blk, 0, sizeof blk);
            if (blk_wr(o, nb, blk) != 0) return -1;
            root.size += VSB_BSIZE;
            if (inode_rw(o, 0, &root, 1) != 0) return -1;
        }
        if (blk_rd(o, (int)root.direct[i], blk) != 0) return -1;
        for (int e = 0; e < DENT_PER_BLOCK; e++) {
            dirent_t *de = (dirent_t *)(blk + (size_t)e * 16);
            if (de->name[0] == 0) {
                memset(de, 0, 16);
                strncpy(de->name, name, VSB_NAMELEN - 1);
                de->inum = (unsigned)free_inum;
                if (blk_wr(o, (int)root.direct[i], blk) != 0) return -1;
                return free_inum;  // the inum, NOT the write status!
            }
        }
    }
    return -1;  // root dir full
}

int vsfs_write(vsfs_ops_t *o, int inum, const unsigned char *data, size_t len) {
    if (check_magic(o) != 0 || len > VSB_MAXFILE) return -1;
    inode_t ino;
    if (inode_rw(o, inum, &ino, 0) != 0 || ino.type != T_FILE) return -1;
    for (int i = 0; i < VSB_NDIRECT; i++) {  // free old blocks first
        if (ino.direct[i] != 0) {
            unsigned char blk[VSB_BSIZE];
            if (o->read(o->ctx, BITMAP_BLOCK, blk) != 0) return -1;
            blk[ino.direct[i] / 8] &= (unsigned char)~(1u << (ino.direct[i] % 8));
            if (o->write(o->ctx, BITMAP_BLOCK, blk) != 0) return -1;
            ino.direct[i] = 0;
        }
    }
    size_t off = 0;
    int bi = 0;
    while (off < len) {
        int nb = bitmap_alloc(o);
        if (nb < 0) return -1;
        unsigned char blk[VSB_BSIZE];
        memset(blk, 0, sizeof blk);
        size_t n = len - off < VSB_BSIZE ? len - off : VSB_BSIZE;
        memcpy(blk, data + off, n);
        if (blk_wr(o, nb, blk) != 0) return -1;
        ino.direct[bi++] = (unsigned)nb;
        off += n;
    }
    ino.size = (unsigned)len;
    return inode_rw(o, inum, &ino, 1);
}

int vsfs_read(vsfs_ops_t *o, int inum, unsigned char *out, size_t cap) {
    inode_t ino;
    if (inode_rw(o, inum, &ino, 0) != 0 || ino.type != T_FILE) return -1;
    size_t n = ino.size < cap ? ino.size : cap;
    size_t off = 0;
    unsigned char blk[VSB_BSIZE];
    for (int i = 0; i < VSB_NDIRECT && off < n; i++) {
        if (ino.direct[i] == 0) break;
        if (blk_rd(o, (int)ino.direct[i], blk) != 0) return -1;
        size_t chunk = n - off < VSB_BSIZE ? n - off : VSB_BSIZE;
        memcpy(out + off, blk, chunk);
        off += chunk;
    }
    return (int)off;
}

int vsfs_list(vsfs_ops_t *o) {
    inode_t root;
    if (inode_rw(o, 0, &root, 0) != 0 || root.type != T_DIR) return -1;
    unsigned char blk[VSB_BSIZE];
    printf("root:");
    for (int i = 0; i < VSB_NDIRECT; i++) {
        if (root.direct[i] == 0) continue;
        if (blk_rd(o, (int)root.direct[i], blk) != 0) return -1;
        for (int e = 0; e < DENT_PER_BLOCK; e++) {
            dirent_t *de = (dirent_t *)(blk + (size_t)e * 16);
            if (de->name[0] != 0) printf(" %s(%u)", de->name, de->inum);
        }
    }
    printf("\n");
    return 0;
}

int vsfs_used_blocks(vsfs_ops_t *o) {
    unsigned char blk[VSB_BSIZE];
    if (blk_rd(o, BITMAP_BLOCK, blk) != 0) return -1;
    int n = 0;
    for (int i = 0; i < VSB_NBLOCKS; i++)
        if ((blk[i / 8] >> (i % 8)) & 1) n++;
    return n;
}

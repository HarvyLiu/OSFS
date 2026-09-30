# VSFS card

- Map: 0 super(magic+counts) 1 bitmap 2-5 inodes(8x32B) 6+ data.
- Inode 32B: type|size|direct[4]|pad. Dirent 16B: name[12]+inum.
- Verbs: mkfs (zero+stamp+protect) create (inode+dirent) write (alloc+fill) read (size-bounded).
- Rules: names unique; len<=256 (direct); free-old-blocks on rewrite.
- Census: used-blocks bitmap count (hello day one: 8/64).
- Next: journal THESE blocks (card becomes journal vocabulary).

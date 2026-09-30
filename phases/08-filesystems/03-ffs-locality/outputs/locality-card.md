# Locality card

- Policies: first-fit (low scan, blind) vs clustered-from-inode (affinity sweep).
- Metrics: span=max-min, gaps=sum|diffs| (14/14 vs 7/7 here).
- Recipe: groups (inode+bitmap+data), files near inodes, dirs near parents, big across groups.
- Orders: sweep glides, nearest-first zigzags (verify, don't assume).
- SSD: seeks dead, caches alive (locality changes currency, never retires).

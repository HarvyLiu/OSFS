# Swap card

- Path: hit (dirty on write) / miss fault / full? evict FIFO (dirty->writeback, clean->drop) / load.
- Numbers (3F/6P trace): 9 faults, 6 evictions, 1 writeback, resident {4,2,5}.
- Rules: presence is a bit; dirt tracked per frame; unmapped range != swapped (fault codes differ).
- OOM: promises > RAM+swap -> killer by score. Tune: oom_score_adj (-1000 never).
- Read: meminfo (pool), ps vsz/rss (promise/residence), dmesg OOM diary.

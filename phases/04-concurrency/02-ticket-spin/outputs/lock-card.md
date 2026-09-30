# Lock card

- Trio: take (fetch_add) -> wait (load-compare+pause) -> advance (fetch_add serving).
- Rules: _Atomic always (plain ints race); load every lap (no caching); pause on x86.
- Costs: spin = user CPU burn (short sections); sleep/futex = switch cost (long waits).
- Discipline: ONE datum ONE lock ALL accessors (04/01's lesson, enforced here).
- Fairness: tickets FIFO free; TAS needs extra work (see Exercises).

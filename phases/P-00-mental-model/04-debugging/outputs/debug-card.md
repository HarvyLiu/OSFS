# Debug card (tape to monitor)

1. Reproduce: killer input vs survivor (./buggy bob vs amy).
2. Build with -g (names + lines survive).
3. gdb -batch -ex run -ex bt --args ./prog killer
4. Read frames bottom-up: library death, your line, your variable.
5. Hypothesize: NULL unchecked? bound off by one?
6. Fix: check at use (!id -> message + exit 2); < for n elements.
7. Pin: assert fails before, passes after (make test green).
8. Loud bugs crash; silent bugs corrupt. Fear the silent ones.

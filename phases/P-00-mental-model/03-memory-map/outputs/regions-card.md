# Regions card

- Code/text: functions (RO). Static: .data init + .bss zero (whole run).
- Heap: malloc..free (return transfers ownership). Stack: {..} frames (NEVER return &local).
- Hybrid: function-static (local scope, global life). Warning return-local-addr = corpse alarm.
- Prove: ASan stack-use-after-return (caged); nm letters T/D/B/R.

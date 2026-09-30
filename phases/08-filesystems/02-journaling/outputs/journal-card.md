# Journal card

- Verbs: stage (home untouched) -> commit flag (point of no return) -> checkpoint (copy home) -> recover (replay iff committed).
- Matrix: crash@0 discard=zeros / crash@1 replay=new / crash@2 noop=new. All CONSISTENT.
- Rules: idempotent replay (twice=once); commit-atomic (whole txn or nothing); order sacred (never checkpoint pre-commit).
- Guards: empty commit refused; over-capacity refused; bad bno refused.
- Read: dmesg recovery lines; tune2fs journal modes (ordered vs writeback).

# Perms card

- Bits: r4 w2 x1 × owner/group/other (octal: 640 = rw-r-----).
- Rule: root yes; else FIRST matching class only (no OR-ing down); test bit.
- Raw: identity AND capability bit (defense in depth).
- setuid: run-as file owner briefly (passwd's rws). Audit sharply.
- Verbs: id, ls -l (rws sighting), capsh (bounding set), EACCES rows in man 2.

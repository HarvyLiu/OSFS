# C strings card

- Decay: using an array as an arg passes `&arr[0]`. Always pass length too.
- Zero rule: strings end at first `'\0'`. `strlen` excludes it, `strcpy` copies it.
- Never `strcpy` into a maybe-small buffer: use bounded copy + force `dst[cap-1]='\0'`.
- Debug: `cc -fsanitize=address,undefined` turns silent smashes into reports.

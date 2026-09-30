# myos tour (final artifact: run this, you ran the course)

1. make && make qemu (Linux; -nographic serial console).
2. Banner: myos> paging+timer+tasks+shell (five subsystems alive).
3. help -> six commands (read the list: shell from 01).
4. ls -> hello.txt about.txt (FS from 08, ROM edition).
5. cat hello.txt -> hello from myos fs (lookup-or-message, P-00/04 grown up).
6. uptime -> ticks climbing ~100/s (PIT from 04, hardware-kept promises).
7. tasks -> spins=N, two esps 4K apart (scheduler from 02/03, evidence attached).
8. tasks again -> spins grew (motion without asking: task two ran while you typed).
9. echo hi -> hi (the shell repeats: Thompson's loop, sixty years young).
10. Ctrl-A X to quit. You booted, paged, ticked, switched, listed, shelled.

# RAMdisk card

- Parity: read/write/flush/crash verbs identical; durability differs (file reloads, RAM wipes).
- Rules: coherence everywhere; flush theater on RAM (counted); crash total; free kills device.
- Uses: initramfs, FS test beds (08 runs here first), speed demos (dd on tmpfs).
- Next: filesystems mount verbs, never media (backend-blind by design).

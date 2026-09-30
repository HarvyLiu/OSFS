// kernel.c -- kmain: VGA + serial "protected! C runs.", then return (stub hangs).
// Lesson 10-boot-to-shell/02-protected-mode/docs/en.md. Freestanding: no libc.
#include <stdint.h>

#define COM1 0x3F8
#define VGA ((volatile uint16_t *)0xB8000)

static inline void outb(uint16_t port, uint8_t v) {
    __asm__ volatile("outb %0, %1" : : "a"(v), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t v;
    __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

static void serial_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

static void serial_putc(char c) {
    while (!(inb(COM1 + 5) & 0x20)) { /* THR empty? poll (no interrupts yet) */ }
    outb(COM1, (uint8_t)c);
}

void kmain(void) {
    const char *msg = "protected! C runs.";
    serial_init();
    for (int i = 0; msg[i]; i++) {
        VGA[i] = (uint16_t)(0x0700 | msg[i]);  // white-on-black cell per char
        serial_putc(msg[i]);
    }
    serial_putc('\n');
}

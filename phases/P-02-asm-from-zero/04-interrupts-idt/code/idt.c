// idt.c -- delivery with masking. Lesson docs/en.md.
#include "idt.h"

static handler_t table[NIRQ];
static int enabled = 1;
static int pending[NIRQ];

void idt_register(int n, handler_t h) { if (n >= 0 && n < NIRQ) table[n] = h; }
void irq_enable(void) { enabled = 1; }
void irq_disable(void) { enabled = 0; }
int irq_pending(int n) { return (n >= 0 && n < NIRQ) ? pending[n] : 0; }

void idt_raise(int n) {
    if (n < 0 || n >= NIRQ || !table[n]) return;
    if (!enabled) { pending[n] = 1; return; }
    pending[n] = 0;
    table[n]();
}

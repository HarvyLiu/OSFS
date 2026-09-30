// demo.c -- tick lifecycle: live, masked, drained. Lesson docs/en.md.
#include <stdio.h>
#include "idt.h"

static int ticks = 0;
static void on_tick(void) { ticks++; }

int main(void) {
    idt_register(0, on_tick);
    irq_enable();
    idt_raise(0);
    idt_raise(0);
    printf("ticks=%d (enabled)\n", ticks);
    irq_disable();
    idt_raise(0);
    printf("ticks=%d pending=%d (masked)\n", ticks, irq_pending(0));
    irq_enable();
    if (irq_pending(0)) { idt_raise(0); }
    printf("ticks=%d after unmask\n", ticks);
    return ticks != 3;
}

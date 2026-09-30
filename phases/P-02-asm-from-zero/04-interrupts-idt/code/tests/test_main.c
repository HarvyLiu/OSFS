// test_main.c -- 5 checks on the real table (links idt.c). Lesson docs/en.md.
#include <assert.h>
#include <stdio.h>
#include "../idt.h"

static int a = 0, b = 0;
static void ha(void) { a++; }
static void hb(void) { b++; }

int main(void) {
    idt_register(0, ha);
    idt_register(1, hb);
    irq_enable();
    idt_raise(0);
    assert(a == 1 && b == 0);       // vector routes, not broadcasts
    idt_raise(7);                   // unregistered: boring, no crash
    assert(a == 1 && b == 0);
    idt_raise(-1);
    idt_raise(99);                  // out of range: ignored
    assert(a == 1 && b == 0);
    irq_disable();
    idt_raise(1);
    assert(b == 0 && irq_pending(1) == 1);  // masked parks
    irq_enable();
    idt_raise(1);
    assert(b == 1);                 // drained after unmask
    printf("all interrupts-idt checks pass\n");
    return 0;
}

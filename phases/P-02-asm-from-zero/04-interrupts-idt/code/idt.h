// idt.h -- phone-book shape. Lesson docs/en.md.
#ifndef OSFS_IDT_H
#define OSFS_IDT_H

#define NIRQ 8
typedef void (*handler_t)(void);
void idt_register(int n, handler_t h);
void idt_raise(int n);
void irq_enable(void);
void irq_disable(void);
int irq_pending(int n);

#endif

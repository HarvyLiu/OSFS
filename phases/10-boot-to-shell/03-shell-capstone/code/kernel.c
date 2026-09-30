// kernel.c -- myos: paging + timer + two tasks + tiny FS + serial shell.
// Note: (uint32_t)(uintptr_t) casts are identity on the -m32 target; the wide
// middle step keeps 64-bit host lint quiet during syntax checks.
// Lesson 10-boot-to-shell/03-shell-capstone/docs/en.md. Freestanding: no libc.
#include <stdint.h>

#define COM1 0x3F8
#define VGA ((volatile uint16_t *)0xB8000)

/* ---------- port + serial ---------- */
static inline void outb(uint16_t p, uint8_t v) {
    __asm__ volatile("outb %0, %1" : : "a"(v), "Nd"(p));
}
static inline uint8_t inb(uint16_t p) {
    uint8_t v;
    __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(p));
    return v;
}
static void serial_init(void) {
    outb(COM1 + 1, 0x00); outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03); outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03); outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}
static void serial_putc(char c) {
    while (!(inb(COM1 + 5) & 0x20)) { }
    outb(COM1, (uint8_t)c);
}
static char serial_getc(void) {
    while (!(inb(COM1 + 5) & 0x01)) { }  // data-ready bit: a key arrived
    return (char)inb(COM1);
}
static void serial_puts(const char *s) {
    for (; *s; s++) serial_putc(*s);
}

/* ---------- libc we wish we had ---------- */
static int kstrcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}
static int kstrncmp(const char *a, const char *b, int n) {
    for (int i = 0; i < n; i++, a++, b++)
        if (*a != *b || !*a) return (int)(unsigned char)*a - (int)(unsigned char)*b;
    return 0;
}
static int kstrlen(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

/* ---------- VGA mirror (eyes) ---------- */
static int vga_pos = 0;
static void vga_putc(char c) {
    if (c == '\n') { vga_pos = (vga_pos / 80 + 1) * 80; }
    else VGA[vga_pos++] = (uint16_t)(0x0700 | (uint8_t)c);
    if (vga_pos >= 80 * 25) vga_pos = 0;  // simplification: wrap, not scroll
}
static void puts2(const char *s) {  // dual output: screen + wire, 10/01's law
    for (const char *p = s; *p; p++) vga_putc(*p);
    serial_puts(s);
}
static void puthex(uint32_t v) {
    char d[8];
    for (int i = 7; i >= 0; i--) {
        int n = (v >> (i * 4)) & 0xF;
        d[7 - i] = (char)(n < 10 ? '0' + n : 'a' + n - 10);
    }
    for (int i = 0; i < 8; i++) { vga_putc(d[i]); serial_putc(d[i]); }
}
static void putdec(uint32_t v) {
    char b[10];
    int i = 0;
    if (!v) { vga_putc('0'); serial_putc('0'); return; }
    while (v) { b[i++] = (char)('0' + v % 10); v /= 10; }
    while (i--) { vga_putc(b[i]); serial_putc(b[i]); }
}

/* ---------- paging: identity-map first 4 MiB (06's tables, alive) ---------- */
static uint32_t page_dir[1024] __attribute__((aligned(4096)));
static uint32_t page_tab[1024] __attribute__((aligned(4096)));
static void paging_init(void) {
    for (int i = 0; i < 1024; i++) page_tab[i] = (uint32_t)(i * 4096) | 3;
    page_dir[0] = (uint32_t)(uintptr_t)page_tab | 3;
    for (int i = 1; i < 1024; i++) page_dir[i] = 0;  // rest unmapped: touch = fault
    __asm__ volatile("movl %0, %%cr3" : : "r"(page_dir));
    uint32_t cr0;
    __asm__ volatile("movl %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u;
    __asm__ volatile("movl %0, %%cr0" : : "r"(cr0));
    __asm__ volatile("jmp 1f\n1:");  // flush prefetch after PG (cheap insurance)
    puts2("paging on: 0-4M identity\n");
}

/* ---------- interrupts: IDT + PIC + PIT 100 Hz (04's world, hardware) ---------- */
struct idt_ent { uint16_t off_lo, sel, zero, flags, off_hi; } __attribute__((packed));
struct idt_ptr { uint16_t limit; uint32_t base; } __attribute__((packed));
static struct idt_ent idt[256];
static struct idt_ptr idtp;
extern void timer_stub(void);
static volatile uint32_t ticks = 0;
static volatile int need_resched = 0;
void timer_handler(void) {
    ticks++;
    if (ticks % 10 == 0) need_resched = 1;  // every 100 ms: contest the CPU
    outb(0x20, 0x20);                       // EOI: master done (slave masked out)
}
static void idt_set(int n, void *h) {
    uint32_t a = (uint32_t)(uintptr_t)h;
    idt[n].off_lo = (uint16_t)(a & 0xFFFF);
    idt[n].sel = 0x08;
    idt[n].zero = 0;
    idt[n].flags = 0x8E;  // present, ring 0, 32-bit interrupt gate
    idt[n].off_hi = (uint16_t)(a >> 16);
}
static void pic_remap(void) {
    outb(0x20, 0x11); outb(0xA0, 0x11);  // ICW1: cascade mode
    outb(0x21, 0x20); outb(0xA1, 0x28);  // ICW2: vectors 0x20 / 0x28 (past CPU faults)
    outb(0x21, 0x04); outb(0xA1, 0x02);  // ICW3: cascade identity
    outb(0x21, 0x01); outb(0xA1, 0x01);  // ICW4: 8086 mode
    outb(0x21, 0xFE); outb(0xA1, 0xFF);  // mask: timer only (keyboard later, maybe)
}
static void pit_100hz(void) {
    outb(0x43, 0x36);                    // channel 0, square wave
    outb(0x40, (1193180 / 100) & 0xFF);  // divisor lo
    outb(0x40, (1193180 / 100) >> 8);    // divisor hi
}
static void irq_init(void) {
    for (int i = 0; i < 256; i++) { idt[i].off_lo = 0; idt[i].flags = 0; }
    idt_set(0x20, timer_stub);           // IRQ0 lands on vector 0x20 (remapped)
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint32_t)(uintptr_t)idt;
    __asm__ volatile("lidt %0" : : "m"(idtp));
    pic_remap();
    pit_100hz();
    __asm__ volatile("sti");
    puts2("timer 100Hz: ticks flow\n");
}

/* ---------- tasks: two stacks, one switch (02's context switch, bare metal) ---------- */
static uint32_t spin_stack[1024];
static uint32_t spin_sp = 0, main_sp = 0;
static volatile uint32_t spins = 0;
extern void switch_to(uint32_t *old_sp, uint32_t new_sp);
static uint32_t read_esp(void) {
    uint32_t v;
    __asm__ volatile("movl %%esp, %0" : "=r"(v));
    return v;
}
static void spinner(void) {
    for (;;) { spins++; switch_to(&spin_sp, main_sp); }  // work, yield, repeat
}
static void task_init(void) {
    spin_sp = (uint32_t)(uintptr_t)&spin_stack[1024];  // stack top (grows down, always)
    spin_sp -= 4;
    *(uint32_t *)(uintptr_t)spin_sp = (uint32_t)(uintptr_t)spinner;  // first ret lands in spinner()
}
static void schedule(void) {
    if (need_resched) {
        need_resched = 0;
        switch_to(&main_sp, spin_sp);  // ride the other stack, come back
    }
    switch_to(&main_sp, spin_sp);  // shell yields every loop (cooperative heart,
                                   // timer-driven flag: hybrid, honestly labeled)
}

/* ---------- tiny FS: ROM files (08's inodes, 30 lines) ---------- */
struct file { const char *name, *data; };
static const struct file fs[] = {
    {"hello.txt", "hello from myos fs\n"},
    {"about.txt", "myos: paging+timer+tasks+shell, one kernel.c\n"},
    {0, 0},
};
static const char *fs_cat(const char *name) {
    for (int i = 0; fs[i].name; i++)
        if (!kstrcmp(fs[i].name, name)) return fs[i].data;
    return 0;
}

/* ---------- shell ---------- */
static char line[128];
static void readline(void) {
    int n = 0;
    for (;;) {
        char c = serial_getc();
        if (c == '\r') { puts2("\n"); line[n] = 0; return; }
        if ((c == 0x08 || c == 0x7F) && n > 0) { n--; puts2("\b \b"); continue; }
        if (n < 127 && c >= 32 && c < 127) { line[n++] = c; vga_putc(c); serial_putc(c); }
    }
}
static void cmd_help(void) {
    puts2("cmds: help ls cat <f> echo <...> uptime tasks\n");
}
static void cmd_ls(void) {
    for (int i = 0; fs[i].name; i++) { puts2(fs[i].name); puts2("\n"); }
}
static void cmd_cat(char *arg) {
    if (!kstrlen(arg)) { puts2("usage: cat <file>\n"); return; }  // empty arg: guide
    const char *d = fs_cat(arg);
    if (d) puts2(d);
    else puts2("no such file\n");
}
static void cmd_echo(char *arg) { puts2(arg); puts2("\n"); }
static void cmd_uptime(void) { putdec(ticks / 100); puts2("s (ticks="); putdec(ticks); puts2(")\n"); }
static void cmd_tasks(void) {
    puts2("spins="); putdec(spins);
    puts2(" main_esp=0x"); puthex(main_sp);
    puts2(" spin_esp=0x"); puthex(spin_sp);
    puts2(" live_esp=0x"); puthex(read_esp());  // same stack as main (a few dozen
    puts2("\n");                                // bytes deeper: call depth, honest)
}
static void dispatch(void) {
    if (!kstrcmp(line, "help")) cmd_help();
    else if (!kstrcmp(line, "ls")) cmd_ls();
    else if (!kstrncmp(line, "cat ", 4)) cmd_cat(line + 4);
    else if (!kstrncmp(line, "echo ", 5)) cmd_echo(line + 5);
    else if (!kstrcmp(line, "uptime")) cmd_uptime();
    else if (!kstrcmp(line, "tasks")) cmd_tasks();
    else if (line[0]) puts2("?: try help\n");
}

void kmain(void) {
    serial_init();
    puts2("myos> paging+timer+tasks+shell\n");
    paging_init();
    irq_init();
    task_init();
    for (;;) {
        schedule();          // let the spinner run (watch spins grow in tasks)
        puts2("myos> ");
        readline();
        dispatch();
    }
}

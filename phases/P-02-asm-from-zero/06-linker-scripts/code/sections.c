// sections.c -- one resident per section class. Lesson docs/en.md.
#include <stdio.h>

int init_global = 41;
int zero_global;
const int ro_const = 7;
int mysec_var __attribute__((section(".mysec"))) = 99;

int main(void) {
    static int stat_init = 5;
    static int stat_zero;
    int local = 1;
    printf("text(main)=%p ro=%p data=%p bss=%p mysec=%p stack~%p\n",
           (void *)main, (const void *)&ro_const, (void *)&init_global,
           (void *)&zero_global, (void *)&mysec_var, (void *)&local);
    printf("values: %d %d %d %d %d %d\n", init_global, zero_global,
           ro_const, mysec_var, stat_init, stat_zero + local);
    (void)stat_init;
    return init_global != 41 || ro_const != 7 || mysec_var != 99;
}

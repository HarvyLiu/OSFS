// sim_regs.c -- C model of the dataflow (no ASM needed).
#include <stdio.h>

int main(void) {
    int edi = 40, esi = 2;
    int eax = edi;
    eax = eax + esi;
    printf("40+2=%d\n", eax);
    return eax != 42;
}

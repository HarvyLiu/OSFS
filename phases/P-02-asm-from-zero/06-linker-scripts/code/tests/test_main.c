// test_main.c -- section-class rules: zero-init, const, custom section value.
#include <assert.h>
#include <stdio.h>

int init_global = 41;
int zero_global;
const int ro_const = 7;
int mysec_var __attribute__((section(".mysec"))) = 99;

int main(void) {
    assert(zero_global == 0);      // .bss starts zeroed, no code needed
    assert(init_global == 41);     // .data carries its initializer
    assert(ro_const == 7);         // .rodata readable (writing it faults)
    assert(mysec_var == 99);       // custom section holds its value
    assert(&mysec_var != &init_global);  // distinct placements
    printf("all linker-scripts checks pass\n");
    return 0;
}

// main.c -- allocator, counter, query in one run. Lesson docs/en.md.
#include <stdio.h>
#include "asmops.h"

int main(void) {
    int s = add_asm(40, 2);
    uint64_t t0 = rdtsc();
    volatile int sink = 0;
    for (int i = 0; i < 1000; i++) sink += i;
    uint64_t t1 = rdtsc();
    char vendor[13];
    cpuid_vendor(vendor);
    printf("add=%d dt=%llu cycles sink=%d vendor=%s\n", s,
           (unsigned long long)(t1 - t0), sink, vendor);
    return s != 42 || t1 <= t0;
}

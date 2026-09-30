// asmops.h -- rdtsc, add-via-template, cpuid vendor. Lesson docs/en.md.
#ifndef OSFS_ASMOPS_H
#define OSFS_ASMOPS_H

#include <stdint.h>

static inline int add_asm(int x, int y) {
    __asm__ ("addl %1, %0" : "+r" (x) : "r" (y) : "cc");
    return x;
}

static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

static inline void cpuid_vendor(char out[13]) {
    uint32_t b, c, d;
    uint32_t a = 0;
    __asm__ volatile ("cpuid"
                      : "=b" (b), "=d" (d), "=c" (c)
                      : "a" (a));
    __builtin_memcpy(out + 0, &b, 4);
    __builtin_memcpy(out + 4, &d, 4);
    __builtin_memcpy(out + 8, &c, 4);
    out[12] = 0;
}

#endif

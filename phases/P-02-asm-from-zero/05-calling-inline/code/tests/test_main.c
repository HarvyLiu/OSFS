// test_main.c -- 5 checks: add_asm math, rdtsc monotonic, vendor shape.
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../asmops.h"

int main(void) {
    assert(add_asm(40, 2) == 42);
    assert(add_asm(0, 0) == 0);
    assert(add_asm(-5, 5) == 0);
    assert(add_asm(-100, -200) == -300);
    uint64_t t0 = rdtsc();
    uint64_t t1 = rdtsc();
    assert(t1 >= t0);  // monotonic (equal allowed on coarse VMs)
    char vendor[13];
    cpuid_vendor(vendor);
    assert(strlen(vendor) == 12);  // "GenuineIntel" / "AuthenticAMD" / hypervisor tag
    printf("all inline-asm checks pass (vendor=%s)\n", vendor);
    return 0;
}

// test_main.c — 3 checks: shifts are mul/div, masks isolate nibbles.
#include <assert.h>
#include <stdio.h>

int main(void) {
    assert((1 << 4) == 16);
    assert((256 >> 3) == 32);
    assert((0xA6 & 0x0F) == 0x06);  // low nibble of A6
    assert(((0xA6 >> 4) & 0x0F) == 0x0A); // high nibble
    printf("all bits checks pass\n");
    return 0;
}

// main.c — P-00/01 bits views. Lesson docs/en.md.
#include <stdio.h>

int main(void) {
    unsigned char b = 0xA6;
    printf("hex=%#x dec=%u bits: ", b, b);
    for (int i = 7; i >= 0; i--) printf("%d", (b >> i) & 1);
    printf("\nmask low-nibble: %#x\n", b & 0x0F);
    printf("1<<4 = %d, 256>>3 = %d\n", 1 << 4, 256 >> 3);
    return 0;
}

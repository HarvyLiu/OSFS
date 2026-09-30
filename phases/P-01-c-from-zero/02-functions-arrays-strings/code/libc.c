// libc.c -- same ideas via string.h (compare with hand-rolled).
#include <stdio.h>
#include <string.h>

int main(void) {
    char dst[16];
    strcpy(dst, "osfs");
    printf("libc: %s %zu %d\n", dst, strlen(dst),
           memcmp(dst, "osfs", 5));
    return 0;
}

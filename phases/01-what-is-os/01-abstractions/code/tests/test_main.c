// test_main.c -- 4 checks: file round-trip via stdio + fd + content + size.
#define _POSIX_C_SOURCE 200809L  // fileno(): glibc hides it under -std=c11 without this
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    const char *path = "test-roundtrip.txt";
    FILE *f = fopen(path, "w");
    assert(f != 0);
    int fd_before_close = fileno(f);
    assert(fd_before_close >= 3);  // 0/1/2 taken: user fds start at 3
    fputs("abc123\n", f);
    fclose(f);

    char buf[64] = {0};
    FILE *r = fopen(path, "r");
    assert(r != 0);
    assert(fgets(buf, sizeof buf, r) != 0);
    fclose(r);
    assert(strcmp(buf, "abc123\n") == 0);
    remove(path);
    printf("all abstractions checks pass\n");
    return 0;
}

// test_main.c -- 4 checks: value semantics, fill, strlen, strcpy-shape.
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int add(int a, int b) { return a + b; }

static void fill(int *arr, int n) {
    for (int i = 0; i < n; i++) arr[i] = i * 10;
}

static unsigned my_strlen(const char *s) {
    unsigned n = 0;
    while (s[n] != '\0') n++;
    return n;
}

int main(void) {
    int x = 5;
    assert(add(x, 3) == 8 && x == 5);  // by-value: caller untouched
    int buf[4];
    fill(buf, 4);
    assert(buf[0] == 0 && buf[3] == 30);
    assert(my_strlen("osfs") == 4);
    assert(my_strlen("") == 0);
    char dst[16];
    strcpy(dst, "osfs");
    assert(memcmp(dst, "osfs", 5) == 0);  // includes NUL
    printf("all fn/array/string checks pass\n");
    return 0;
}

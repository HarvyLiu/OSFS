// main.c -- P-01/02 values vs arrays vs strings. Lesson docs/en.md.
#include <stdio.h>

int add(int a, int b) { return a + b; }

void fill(int *arr, int n) {
    for (int i = 0; i < n; i++) arr[i] = i * 10;
}

unsigned my_strlen(const char *s) {
    unsigned n = 0;
    while (s[n] != '\0') n++;
    return n;
}

int main(void) {
    int x = 5;
    int s = add(x, 3);
    printf("add: x=%d s=%d\n", x, s);

    int buf[4];
    fill(buf, 4);
    printf("buf: %d %d %d %d\n", buf[0], buf[1], buf[2], buf[3]);

    char name[] = "osfs";
    printf("name=%s len=%u\n", name, my_strlen(name));
    return 0;
}

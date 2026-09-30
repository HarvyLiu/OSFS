// main.c -- caller of framed add. Lesson docs/en.md.
#include <stdio.h>

int frame_add(int a, int b);

int main(void) {
    int r = frame_add(30, 12);
    printf("frame_add(30,12)=%d\n", r);
    return r != 42;
}

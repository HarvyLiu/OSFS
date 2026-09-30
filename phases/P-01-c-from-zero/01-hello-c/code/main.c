// main.c — P-01/01 hello + control flow. Lesson docs/en.md.
#include <stdio.h>

int square(int x) {
    return x * x;
}

int main(void) {
    for (int i = 0; i < 5; i++) {
        if (i % 2 == 0)
            printf("i=%d square=%d (even)\n", i, square(i));
        else
            printf("i=%d square=%d\n", i, square(i));
    }
    return 0;
}

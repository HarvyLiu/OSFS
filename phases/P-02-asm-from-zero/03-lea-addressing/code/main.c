// main.c -- caller of the three addressing shapes. Lesson docs/en.md.
#include <stdio.h>

int scaled_get(int *b, int i);
int mul3add(int x);
int field_state(void *p);

struct rec { int pid; int state; };

int main(void) {
    int arr[4] = {10, 20, 30, 40};
    struct rec p = {5, 2};
    int a = scaled_get(arr, 2), m = mul3add(7), s = field_state(&p);
    printf("scaled=%d mul3=%d state=%d\n", a, m, s);
    return a != 30 || m != 21 || s != 2;
}

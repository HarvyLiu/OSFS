// sim_addr.c -- standalone C mirror (also feeds gcc -S demo).
#include <stdio.h>

int main(void) {
    int arr[4] = {10, 20, 30, 40};
    printf("arr[2]=%d x3=%d\n", arr[2], 7 + 7 * 2);
    return 0;
}

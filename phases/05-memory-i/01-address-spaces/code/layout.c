// layout.c -- address-space selfie: code/rodata/data/bss/heap/stack. Lesson docs/en.md.
#include <stdio.h>
#include <stdlib.h>

int init_global = 41;
int zero_global;

int main(void) {
    int stack_var = 1;
    void *heap1 = malloc(16);
    void *heap2 = malloc(16);
    if (!heap1 || !heap2) return 1;
    printf("code(main)=%p rodata=%p data=%p bss=%p\n",
           (void *)main, (void *)"lit", (void *)&init_global, (void *)&zero_global);
    printf("heap1=%p heap2=%p gap=%ld\n", heap1, heap2,
           (long)((char *)heap2 - (char *)heap1));
    printf("stack~%p heap-below-stack=%d code-below-heap=%d (Linux canon: 1 1; Windows differs)\n",
           (void *)&stack_var, (void *)&stack_var > heap1, heap1 > (void *)main);
    free(heap1);
    free(heap2);
    return 0;
}

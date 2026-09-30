// demo.c -- file round-trip + address-space selfie. Lesson docs/en.md.
#include <stdio.h>
#include <string.h>

int main(void) {
    const char *path = "build/hello-osfs.txt";
    FILE *f = fopen(path, "w");
    if (!f) { perror("fopen w"); return 1; }
    fprintf(f, "hello from an abstraction\n");
    fclose(f);

    char buf[64];
    FILE *r = fopen(path, "r");
    if (!r) { perror("fopen r"); return 1; }
    if (!fgets(buf, sizeof buf, r)) { fprintf(stderr, "empty?\n"); return 1; }
    fclose(r);

    int stack_var = 1;
    printf("read back: %s", buf);
    printf("file=%s stack~%p code~%p\n", path, (void *)&stack_var, (void *)main);
    return strcmp(buf, "hello from an abstraction\n") != 0;
}

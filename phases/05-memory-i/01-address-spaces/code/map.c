// map.c -- anonymous pages, zeroed, mine. Lesson docs/en.md. Linux-only.
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
    long ps = sysconf(_SC_PAGESIZE);
    size_t len = (size_t)ps * 2;
    char *p = mmap(0, len, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); return 1; }
    p[0] = 'A';
    p[len - 1] = 'Z';
    printf("pagesize=%ld len=%zu map=%p first=%c last=%c\n", ps, len, (void *)p, p[0], p[len - 1]);
    munmap(p, len);
    return ps != 4096;
}

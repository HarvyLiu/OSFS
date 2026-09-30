// pid.c -- fd-level I/O: open/write/read/close + getpid. Lesson docs/en.md.
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int main(void) {
    long pid = (long)getpid();
    const char *path = "build/trap-demo.txt";
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open"); return 1; }
    const char *msg = "knock knock (via write)\n";
    size_t left = strlen(msg);
    while (left > 0) {
        ssize_t n = write(fd, msg + (strlen(msg) - left), left);
        if (n < 0) { perror("write"); return 1; }
        left -= (size_t)n;
    }
    close(fd);

    char buf[64];
    int rfd = open(path, O_RDONLY);
    if (rfd < 0) { perror("reopen"); return 1; }
    ssize_t nr = read(rfd, buf, sizeof(buf) - 1);
    if (nr < 0) { perror("read"); return 1; }
    close(rfd);
    buf[nr] = 0;
    printf("pid=%ld back=[%s]", pid, buf);
    return strcmp(buf, msg) != 0;
}

// test_main.c -- 4 checks: pid sane, fd round-trip, short-write loop shape, size.
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int main(void) {
    assert(getpid() > 0);  // identity papers exist
    const char *path = "test-trap.txt";
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert(fd >= 3);  // 0/1/2 taken
    const char *msg = "trap-test-123";
    size_t left = strlen(msg), off = 0;
    while (left > 0) {  // robust loop, same shape as demo
        ssize_t n = write(fd, msg + off, left);
        assert(n > 0);
        off += (size_t)n;
        left -= (size_t)n;
    }
    close(fd);
    char buf[64] = {0};
    int rfd = open(path, O_RDONLY);
    assert(rfd >= 0);
    ssize_t nr = read(rfd, buf, sizeof(buf) - 1);
    close(rfd);
    assert(nr == (ssize_t)strlen(msg));
    assert(strcmp(buf, msg) == 0);
    remove(path);
    printf("all syscalls-strace checks pass\n");
    return 0;
}

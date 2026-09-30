// test_main.c — fork/exit-status checks (Linux only).
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    pid_t pid = fork();
    assert(pid >= 0);
    if (pid == 0) _exit(42);          // child: exit code path
    int status = 0;
    assert(waitpid(pid, &status, 0) == pid);
    assert(WIFEXITED(status));        // exited normally
    assert(WEXITSTATUS(status) == 42);// status round-trips via PCB
    printf("all fork checks pass\n");
    return 0;
}

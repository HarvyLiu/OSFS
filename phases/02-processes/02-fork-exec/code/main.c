// main.c — 02 fork/exec/wait demo (Linux/WSL/Docker only).
// Lesson: phases/02-processes/02-fork-exec/docs/en.md
// Hosted C; bare-metal myos will reimplement PCB + trap path later.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }
    if (pid == 0) {
        execlp("echo", "echo", "hello-from-child", (char *)NULL);
        perror("exec");
        _exit(127);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) { perror("waitpid"); return 1; }
    printf("child %d exited=%d\n", (int)pid, WEXITSTATUS(status));
    return 0;
}

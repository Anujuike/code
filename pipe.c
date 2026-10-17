#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    int p1[2], p2[2];
    pid_t pid;

    if (pipe(p1) < 0 || pipe(p2) < 0) {
        perror("pipe");
        exit(1);
    }

    pid = fork();

    if (pid == 0) {
        dup2(p1[1], STDOUT_FILENO);
        close(p1[0]);
        close(p1[1]);
        close(p2[0]);
        close(p2[1]);
        execlp("head", "head", "/etc/passwd", (char *)0);
        perror("head");
        exit(1);
    }

    pid = fork();

    if (pid == 0) {
        dup2(p1[0], STDIN_FILENO);
        dup2(p2[1], STDOUT_FILENO);
        close(p1[0]);
        close(p1[1]);
        close(p2[0]);
        close(p2[1]);
        execlp("tail", "tail", "-3", (char *)0);
        perror("tail");
        exit(1);
    }

    pid = fork();

    if (pid == 0) {
        dup2(p2[0], STDIN_FILENO);
        close(p1[0]);
        close(p1[1]);
        close(p2[0]);
        close(p2[1]);
        execlp("wc", "wc", "-l", (char *)0);
        perror("wc");
        exit(1);
    }

    close(p1[0]);
    close(p1[1]);
    close(p2[0]);
    close(p2[1]);

    wait(0);
    wait(0);
    wait(0);

    return 0;
}

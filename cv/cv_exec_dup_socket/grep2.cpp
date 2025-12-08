#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

int main()
{

    int fd1[2];
    int fd2[2];
    pipe(fd1);
    pipe(fd2);

    pid_t p1 = fork();
    if (p1 == 0)
    {
        //1. child - cat
        close(fd1[0]);
        dup2(fd1[1], STDOUT_FILENO);
        close(fd1[1]);
        close(fd2[0]);
        close(fd2[1]);
        execlp("cat", "cat", "main.cpp", NULL);
        perror("exec");
        _exit(1);
    }

    pid_t p2 = fork();
    if (p2 == 0)
    {
        //2. child - grep
        close(fd1[1]);
        dup2(fd1[0], STDIN_FILENO);
        dup2(fd2[1], STDOUT_FILENO);
        close(fd1[0]);
        close(fd2[0]);
        close(fd2[1]);
        execlp("grep", "grep", "include", NULL);
        perror("exec");
        _exit(1);
    }

    pid_t p3 = fork();
    if (p3 == 0)
    {
        //3. child - wc
        close(fd2[1]);
        dup2(fd2[0], STDIN_FILENO);
        close(fd2[0]);
        close(fd1[0]);
        close(fd1[1]);
        execlp("wc", "wc", "-l", NULL);
        perror("exec");
        _exit(1);
    }
    
    close(fd1[0]);
    close(fd1[1]);
    close(fd2[0]);
    close(fd2[1]);
    wait(NULL);
    wait(NULL);

    return 0;
}
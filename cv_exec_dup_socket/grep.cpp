#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

int main(void)
{

    int fd_cat[2];
    int fd_grep[2];
    const char *f = "main.cpp";
    const char *grep = "include";

    if (pipe(fd_cat) == -1)
    {
        perror("pipe failed");
    }

    pid_t p_cat = fork();

    if (p_cat == -1)
    {
        perror("fork error");
    }
    if (p_cat == 0)
    {
        // child
        int fd[2];
        pipe(fd);
        pid_t p = fork();
        if (p == 0)
        {
            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
            execlp("wc", "wc", "-l", NULL);
            _exit(1);
        }
        else
        {
            close(fd[0]);
            close(fd_cat[1]);
            dup2(fd_cat[0], STDIN_FILENO);
            close(fd_cat[0]);
            execlp("grep", "grep", grep, NULL);
            perror("exec grep");
            _exit(1);
        }
    }
    if (p_cat > 0)
    {
        // parent
        close(fd_cat[0]);
        dup2(fd_cat[1], STDOUT_FILENO);
        close(fd_cat[1]);
        sleep(1);
        execlp("cat", "cat", f, NULL);
        perror("exec cat");
        _exit(1);
    }

    // wc fork()

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>


int main(void) {
    int fd[2];
    pipe(fd);

    pid_t pid = fork();

    if (pid == 0) {
        // ---- DIEŤA ----
        close(fd[1]);                     // zavrie zapisovací koniec
        dup2(fd[0], STDIN_FILENO);        // stdin = pipe čítací koniec
        close(fd[0]);                     // nepotrebujeme dupnutý deskriptor
        execlp("wc", "wc", "-l", NULL);   // spustí „wc -l“
        perror("exec wc");
        _exit(1);
    } else {
        // ---- RODIČ ----
        close(fd[0]);                     // zavrie čítací koniec
        dup2(fd[1], STDOUT_FILENO);       // stdout = pipe zapisovací koniec
        close(fd[1]);
        sleep(1);
        execlp("ls", "ls", "-a", NULL);         // spustí „ls“
        perror("exec ls");
        _exit(1);
    }

    return 0;
}
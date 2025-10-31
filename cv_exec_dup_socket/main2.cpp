#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>


int main() {

    int fd[2];
    pipe(fd);
    pid_t p = fork();

    if (p == 0)
    {
        close(fd[1]);
        char buf[64];
        read(fd[0], buf, sizeof(buf));
        printf("Dieta cita: %s\n", buf);
        _exit(0);
    }

    close(fd[0]);
    printf("Rodic posiela spravu Ahoj\n");
    write(fd[1], "Ahoj", strlen("Ahoj"));
    close(fd[1]);
    wait(NULL);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

/* P2 – Pipe + fork (rodič → dieťa)  (priprava)

Súbor: p02_parent_to_child.cpp
Cieľ: rodič zapíše do rúry text, dieťa ho prečíta a vypíše.

Špecifikácia (prečítaj, napíšeme po P1)
	•	pipe(fd), fork()
	•	Rodič: zavrie fd[0], zapíše „Ahoj Dávid“, zavrie fd[1], wait(NULL).
	•	Dieťa: zavrie fd[1], číta z fd[0], vypíše Dieta dostalo: ..., zavrie fd[0], exit(0). */

int main() {
    int fd[2];
    if (pipe(fd) == -1)
    {
        perror("pipe");
        return 1;
    }

    pid_t p = fork();
    if (p == -1)
    {
        perror("fork");
        return 1;
    }

    if (p > 0)
    {
        //parent
        close(fd[0]);
        const char *msg = "Ahoj David";
        size_t w = write(fd[1], msg, strlen(msg));
        if (w == -1)
        {
            perror("write");
            close(fd[1]);

            return 1;
        }
        close(fd[1]);
        wait(NULL);
    }
    if (p == 0)
    {
        //child
        close(fd[1]);
        char buf[64];
        size_t r = read(fd[0], buf, sizeof(buf));
        if(r == -1) {perror("read"); close(fd[0]); return 1;}
        close(fd[0]);
        printf("Dieta dostalo: %.*s\n", (int)r, buf);
        exit(0);
    }

    return 0;
}
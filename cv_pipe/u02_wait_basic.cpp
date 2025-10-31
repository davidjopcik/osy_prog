#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {

    pid_t p = fork();

    if (p<0)
    {
        perror("fork error");
        return 1;
    }
    else if (p > 0)
    {
        //parent
        wait(NULL);
        printf("Dieta skoncilo\n");
    }
    else if (p == 0)
    {
        printf("Dieta start: PID=%d...\n", getpid());
        sleep(3);
        printf("Dieta koniec\n");
        return 0;
    } 

    return 0;
}
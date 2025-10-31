#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {

    pid_t p;
    p = fork();

    if (p < 0)
    {
        perror("fork");
        return 1;
    }
    
    if (p > 0)
    {
        printf("PID rodica=%d\n", getpid());
        printf("PID dietata=%d\n", p);
    }
    else if (p == 0)
    {
        printf("PID dietata=%d\n", getpid());
        printf("PPID dietata=%d\n", getppid());
    }
    return 0;
}
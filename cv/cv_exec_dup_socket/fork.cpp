#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int main() {

    pid_t p[2];

    for (int i = 0; i < 2; i++)
    {
        p[i] = fork();
        if (p[i] == 0)
        {
            //child
            printf("[child %d]My PID=%d\n", i, getpid());
            sleep(1);
            _exit(0);
        }
    }
    for (int i = 0; i < 2; i++)
    {
        wait(NULL);
    }
    
    printf("Vsetky deti skoncili, rodic konci...\n");

    return 0;
}
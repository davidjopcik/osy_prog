#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>

int main(int argn, char **argc)
{
 /*     printf("Zaciname...\n");
    for (int i = 0; i < 10; i++)
    {
        if (fork() == 0)
            break;
    }
    printf("PID %d\n", getpid()); */
    //getchar(); 

    int roura[2];
    pipe(roura);

    if (fork() == 0)
    {
        close(roura[0]);
        printf("Potomek PID %d\n", getpid());
        for (int i = 0; i < 10; i++)
        {
            char buf[1313];
            sprintf(buf, "%d\n", rand() % 100000);
            write(roura[1], buf, strlen(buf));
            usleep(500000); // 0.5s
        }
        close(roura[1]);
    }
    else
    {
        close(roura[1]);
        while (1)
        {
            char buf[111];
            int r = read(roura[0], buf, sizeof(buf));
            if (r == 0)
                break;
            write(1, buf, r);
        }
        close(roura[0]);
        printf("Rodic PID %d\n", getpid());
        wait(NULL);
        // getchar();
        printf("Rodic konci...\n");
    }
}
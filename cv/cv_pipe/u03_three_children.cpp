#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {

    pid_t p;
    const int N = 3;

    for (int i = 0; i < N; i++)
    {
        p = fork();
        if (p == -1)
        {
            perror("fork failed");
        }
        if (p == 0)
        {
            //dieta
            printf("[child] i=%d PID=%d PPID=%d\n", i, getpid(), getppid());
            exit(10+i);
        }
        else
        {
            //rodic
            printf("[parent] PID=%d vytvoril som dieta %d\n", getpid(), p);
        }
    }

    //parent wait for all N child
        for (int i = 0; i < N; i++)
        {
            int status = 0;
            pid_t child = wait(&status);
            if(WIFEXITED(status)){
                printf("[parent] ukoncene dieta PID = %d exit=%d\n", child, WEXITSTATUS(status));
            }
            else
            {
                printf("[parent] dieta PID=%d neskopncilo normalne\n", child);
            }
        } 
    
    return 0;
}
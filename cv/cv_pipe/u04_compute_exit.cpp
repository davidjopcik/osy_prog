#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        fprintf(stderr, "Neplatny pocet argumentov\n");
        return 1;
    }

    char *N_end{};
    int N = strtol(argv[1], &N_end, 10);
    if (N > 50 || N < 0)
    {
        fprintf(stderr, "Cislo nie je v rozsahu\n");
        return 1;
    }

    int S = 0;
    pid_t p = fork();

    if (p == -1)
    {
        perror("fork zlyhal");
        return 1;
    }

    if (p == 0)
    {
        // child
        for (int i = 0; i <= N; i++)
        {
            S += i;
        }
        exit(S % 256);
    }
    if (p > 0)
    {
        // parent
        int status = 0;
        wait(&status);
        if (WIFEXITED(status))
        {
            int code = WEXITSTATUS(status);
            printf("Suma(0..%d) mod 256 = %d\n", N, code);
        }
    }

    return 0;
}
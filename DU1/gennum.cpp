#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <string.h>

void generatorNum(long *S, int *N, int binary)
{
    long newNum = *S;
    for (int i = 0; i < *N; i++)
    {
        newNum += 1;
        if (binary) write(STDOUT_FILENO, &newNum, sizeof(newNum));
        else        printf("%ld\n", newNum);
    }
}

int main(int argc, char *argv[])
{
    long S;
    int N = 1000;
    int binary = 0;

    if (argc > 4 || argc < 2)
    {
        fprintf(stderr, "Neplatny pocet argumentov!\n");
        return 1;
    }

    sscanf(argv[1], "%ld", &S);

        for (int i = 2; i < argc; i++)
        {
            if (argv[i][0] == '-' && argv[i][1] == 'b') binary = 1;
            else sscanf(argv[2], "%d", &N);
        }

    generatorNum(&S, &N, binary);

    return 0;
}
#include <stdio.h>
#include <unistd.h>
#include "gen.h"

void generatorNum(long S, int N, int binary)
{
    long newNum = S;
    for (int i = 0; i < N; i++)
    {
        newNum += 1;
        if (binary)
            write(STDOUT_FILENO, &newNum, sizeof(newNum));
        else
            printf("%010ld\n", newNum);
    }
}
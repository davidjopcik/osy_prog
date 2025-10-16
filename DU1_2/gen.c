#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include "gen.h"

void generator(long S, long N, bool binary)
{
    long num = S;
    for (int i = 0; i < N; i++)
    {
        if (binary)
        {
            write(1, &num, sizeof(num));
            printf("\n");
        }
        else
        {
            printf("%010ld\n", num);
        }

        num += 1;
    }
}
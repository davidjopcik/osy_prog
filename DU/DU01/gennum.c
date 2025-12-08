#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "gen.h"


int main(int argc, char *argv[])
{
    char *end;
    long S = strtol(argv[1], &end, 10);
    long N;
    bool binary = false;

    if (argc == 2 )
    {
        N = 1000;
    }
    if (argc > 2)
    {
        if (strcmp(argv[2], "-b") == 0)
        {
            N = 1000;
        }
        else
        {
            N = atoi(argv[2]);
        }
        
    }
        
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-b") == 0)
        {
            binary = true;
        }
    }

    generator(S, N, binary);

    return 0;
}
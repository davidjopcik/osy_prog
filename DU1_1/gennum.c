#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gen.h"

int main(int argc, char *argv[])
{
    if (argc < 2 || argc > 4) {
        fprintf(stderr, "Neplatny pocet argumentov!\n");
        return 1;
    }

    long S = 0;
    int  N = 1000;
    int  binary = 0;

    if (sscanf(argv[1], "%ld", &S) != 1) {
        fprintf(stderr, "Zly format S\n");
        return 1;
    }

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-b") == 0) binary = 1;
        else N = atoi(argv[i]);
    }

    generatorNum(S, N, binary);
    return 0;
}
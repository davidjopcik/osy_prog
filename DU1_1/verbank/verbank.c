#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "./verlib/ver.h"

int main(int argc, char *argv[]) {
    long weight[10] = {1, 2, 4, 8, 5, 10, 9, 7, 3, 6};
    long S;
    char *end;
    char line[11];
    bool onlyValid = false;

    if ((argc == 2) && (strcmp(argv[1], "-v") == 0))
    {
        onlyValid = true;
    }

    while (scanf("%10s", line) == 1)  
    {
        S = strtol(line, &end, 10);
        verification(S, weight, onlyValid);
    }

    return 0;
}